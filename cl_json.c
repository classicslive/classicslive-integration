#include "cl_common.h"
#include "cl_json.h"
#include "cl_memory.h"
#include "3rdparty/jsonsax/jsonsax.h"

#include <string/stdstring.h>

#include <stdio.h>

typedef enum
{
  CL_JSON_STATE_INVALID = 0,

  CL_JSON_STATE_STARTING,
  CL_JSON_STATE_KEY_FOUND,
  CL_JSON_STATE_ARRAY_STARTED,
  CL_JSON_STATE_ARRAY_ENDED,
  CL_JSON_STATE_FINISHED,
  CL_JSON_STATE_ERROR,

  CL_JSON_STATE_SIZE
} cl_json_state;

typedef struct cl_json_t
{
  /** The buffer to write our value into */
  void *data;

  /** The JSON key that reperesents the value we want */
  const char *key;

  /** Internal; whether the currently processed JSON element is what we want */
  bool is_current;

  /** Internal; whether the currently processed JSON element is in an array */
  unsigned array_level;

  /** Internal; whether the currently processed JSON element is an object */
  bool is_object;

  /** Whether or not the requested element was copied into the buffer */
  cl_json_state state;

  /** The size, in bytes, of the buffer to be written into */
  unsigned size;

  /** The type of JSON data we are looking for */
  cl_json_type type;

  /** The current field when decoding an object */
  cl_json_field field;

  /** In array decoding, the index of the current element */
  unsigned element_num;

  /** In array decoding, the total number of elements */
  unsigned element_count;
} cl_json_t;

typedef struct cl_json_key_map_t
{
  cl_json_field field;
  const char *name;
} cl_json_key_map_t;

static const cl_json_key_map_t cl_json_key_map[] =
{
  { CL_JSON_KEY_ACHIEVEMENTS,   "achievements"   },
  { CL_JSON_KEY_ADDRESS,        "address"        },
  { CL_JSON_KEY_DESCRIPTION,    "description"    },
  { CL_JSON_KEY_DETAILS,        "details"        },
  { CL_JSON_KEY_ENDIANNESS,     "endianness"     },
  { CL_JSON_KEY_FLAGS,          "flags"          },
  { CL_JSON_KEY_GAME_ID,        "game_id"        },
  { CL_JSON_KEY_ICON_URL,       "icon_url"       },
  { CL_JSON_KEY_ID,             "id"             },
  { CL_JSON_KEY_LEADERBOARDS,   "leaderboards"   },
  { CL_JSON_KEY_MEMORY_NOTE_ID, "memory_note_id" },
  { CL_JSON_KEY_MEMORY_NOTES,   "memory_notes"   },
  { CL_JSON_KEY_OFFSETS,        "offsets"        },
  { CL_JSON_KEY_ORDER,          "order"          },
  { CL_JSON_KEY_POINTER_SIZE,   "pointer_size"   },
  { CL_JSON_KEY_REASON,         "reason"         },
  { CL_JSON_KEY_SCRIPT,         "script"         },
  { CL_JSON_KEY_SESSION_ID,     "session_id"     },
  { CL_JSON_KEY_SUCCESS,        "success"        },
  { CL_JSON_KEY_TITLE,          "title"          },
  { CL_JSON_KEY_TYPE,           "type"           },
  { CL_JSON_KEY_UNLOCKED,       "unlocked"       },

  { CL_JSON_KEY_NONE,           NULL             }
};

static const char *cl_json_key_name(cl_json_field field)
{
  const cl_json_key_map_t *map;

  for (map = cl_json_key_map; map->name != NULL; map++)
  {
    if (map->field == field)
      return map->name;
  }

  return NULL;
}

static cl_json_field cl_json_key_field(const char *name, size_t length)
{
  const cl_json_key_map_t *map;

  for (map = cl_json_key_map; map->name != NULL; map++)
  {
    if (!strncmp(map->name, name, length))
      return map->field;
  }

  return CL_JSON_KEY_NONE;
}

static int cl_json_key(void *userdata, const char *name, size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->state != CL_JSON_STATE_FINISHED &&
      !ud->array_level &&
      !ud->is_object)
  {
    ud->is_current = !strncmp(ud->key, name, length);
    if (ud->is_current)
      ud->state = CL_JSON_STATE_KEY_FOUND;
  }

  return 0;
}

static int cl_json_key_array(void *userdata, const char *name, size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->state == CL_JSON_STATE_ARRAY_STARTED)
    ud->field = cl_json_key_field(name, length);
  else if (ud->state != CL_JSON_STATE_FINISHED)
  {
    ud->is_current = !strncmp(ud->key, name, length);
    if (ud->is_current)
      ud->state = CL_JSON_STATE_KEY_FOUND;
  }

  return 0;
}

static int cl_json_boolean(void *userdata, int istrue)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current)
  {
    ud->is_current = false;
    switch (ud->type)
    {
    case CL_JSON_TYPE_BOOLEAN:
      *((bool*)ud->data) = istrue ? true : false;
      ud->state = CL_JSON_STATE_FINISHED;
      break;
    default:
      return 1;
    }
  }

  return 0;
}

static int cl_json_boolean_array(void *userdata, int istrue)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current && ud->field)
  {
    switch (ud->type)
    {
    case CL_JSON_TYPE_ACHIEVEMENT:
    {
      cl_achievement_t *ach = &((cl_achievement_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_UNLOCKED:
        ach->unlocked = istrue ? true : false;
        break;
      default:
        break;
      }
      ud->field = CL_JSON_KEY_NONE;
      break;
    }
    default:
      return 1;
    }
  }

  return 0;
}

static int cl_json_number(void *userdata, const char* number, size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;
  CL_UNUSED(length);

  if (ud->is_current)
  {
    ud->is_current = false;
    switch (ud->type)
    {
    case CL_JSON_TYPE_NUMBER:
      ud->state = cl_strto(&number, ud->data, ud->size, false) ?
                           CL_JSON_STATE_FINISHED : CL_JSON_STATE_ERROR;
      break;
    default:
      return 1;
    }
  }

  return 0;
}

static int cl_json_number_array(void *userdata, const char* number,
  size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;
  unsigned value = 0;
  bool success = cl_strto(&number, &value, sizeof(value), false);
  CL_UNUSED(length);

  if (!success)
    return 1;
  else if (ud->is_current && ud->field)
  {
    switch (ud->type)
    {
    case CL_JSON_TYPE_ACHIEVEMENT:
    {
      cl_achievement_t *ach = &((cl_achievement_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_ID:
        ach->id = value;
        break;
      case CL_JSON_KEY_FLAGS:
        ach->flags = value;
        break;
      default:
        break;
      }
      break;
    }
    case CL_JSON_TYPE_LEADERBOARD:
    {
      cl_leaderboard_t *ldb = &((cl_leaderboard_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_ID:
        ldb->id = value;
        break;
      default:
        break;
      }
      break;
    }
    case CL_JSON_TYPE_MEMORY_NOTE:
    {
      cl_memnote_t *note = &((cl_memnote_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_ADDRESS:
        note->address_initial = value;
        break;
      case CL_JSON_KEY_FLAGS:
        note->flags = value;
        break;
      case CL_JSON_KEY_ID:
        note->key = value;
        break;
      case CL_JSON_KEY_ORDER:
        note->order = value;
        break;
      case CL_JSON_KEY_TYPE:
        note->type = (cl_value_type)value;
        break;
      default:
        break;
      }
      break;
    }
    default:
      return 1;
    }
    ud->field = CL_JSON_KEY_NONE;
  }

  return 0;
}

static int cl_json_start_array(void *userdata)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current)
    ud->state = CL_JSON_STATE_ARRAY_STARTED;
  ud->array_level++;

  return 0;
}

static int cl_json_end_array(void *userdata)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current)
  {
    ud->state = CL_JSON_STATE_FINISHED;
    ud->is_current = false;
  }
  ud->array_level--;

  return 0;
}

static int cl_json_array_index(void *userdata, unsigned index)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current && index < ud->element_count)
    ud->element_num = index;

  return 0;
}

static int cl_json_array_index_count(void *userdata, unsigned index)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current)
    ud->element_count = index + 1;

  return 0;
}

static void unescape(char *s)
{
  char *src = s;
  char *dst = s;

  while (*src)
  {
    if (*src == '\\')
    {
      src++;
      switch (*src)
      {
        case 'n': *dst++ = '\n'; break;
        case 'r': *dst++ = '\r'; break;
        case 't': *dst++ = '\t'; break;
        case 'b': *dst++ = '\b'; break;
        case 'f': *dst++ = '\f'; break;
        case 'v': *dst++ = '\v'; break;
        case 'a': *dst++ = '\a'; break;
        case '\\': *dst++ = '\\'; break;
        case '\'': *dst++ = '\''; break;
        case '"': *dst++ = '"'; break;
        case 'x':
        {
          int val = 0;
          int i;

          src++;
          for (i = 0; i < 2 && isxdigit((unsigned char)*src); i++, src++)
          {
            val = val * 16 + (isdigit(*src) ? *src - '0' :
                              (tolower(*src) - 'a' + 10));
          }
          *dst++ = (char)val;
          src--;
          break;
        }
        default:
          *dst++ = '\\';
          *dst++ = *src;
          break;
      }
      src++;
    }
    else
      *dst++ = *src++;
  }
  *dst = '\0';
}

static void cl_json_strcpy(char *dst, size_t dstlen, const char *src,
  size_t srclen)
{
  snprintf(dst, dstlen, "%s", src);
  dst[srclen] = '\0';
  unescape(dst);
}

static bool cl_json_parse_offsets(cl_memnote_t *note, const char *string)
{
  unsigned count = 0;
  const char *ptr = string;
  unsigned i;

  /* Parse number of offsets */
  if (!cl_strto(&ptr, &count, sizeof(count), false))
    return false;
  else if (count > CL_POINTER_MAX_PASSES)
    return false;

  note->pointer_passes = count;

  /* Parse each offset */
  for (i = 0; i < count; i++)
  {
    unsigned value = 0;

    /* Skip whitespace */
    while (*ptr == ' ' || *ptr == '\t')
      ptr++;

    if (!cl_strto(&ptr, &value, sizeof(value), false))
      return false;

    note->pointer_offsets[i] = value;
  }

  return true;
}

static int cl_json_string(void *userdata, const char *string, size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current || ud->field)
  {
    ud->is_current = false;
    switch (ud->type)
    {
    case CL_JSON_TYPE_BOOLEAN:
      if (string_is_equal(ud->data, "true"))
        *((bool*)ud->data) = true;
      else
        *((bool*)ud->data) = false;
      break;
    case CL_JSON_TYPE_NUMBER:
      cl_strto(&string, ud->data, ud->size, false);
      break;
    case CL_JSON_TYPE_STRING:
      cl_json_strcpy(ud->data, ud->size, string, length);
      break;
    default:
      return 1;
    }
    ud->state = CL_JSON_STATE_FINISHED;
  }

  return 0;
}

static int cl_json_string_array(void *userdata, const char *string,
  size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->is_current && ud->field)
  {
    switch (ud->type)
    {
    case CL_JSON_TYPE_ACHIEVEMENT:
    {
      cl_achievement_t *ach = &((cl_achievement_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_DESCRIPTION:
        cl_json_strcpy(ach->description, sizeof(ach->description), string,
                       length);
        break;
      case CL_JSON_KEY_ICON_URL:
        cl_json_strcpy(ach->icon_url, sizeof(ach->icon_url), string, length);
        break;
      case CL_JSON_KEY_TITLE:
        cl_json_strcpy(ach->title, sizeof(ach->title), string, length);
        break;
      default:
        return 1;
      }
      break;
    }
    case CL_JSON_TYPE_LEADERBOARD:
    {
      cl_leaderboard_t *ldb = &((cl_leaderboard_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_DESCRIPTION:
        cl_json_strcpy(ldb->description, sizeof(ldb->description), string,
                       length);
        break;
      case CL_JSON_KEY_DETAILS:
        cl_json_strcpy(ldb->details, sizeof(ldb->details), string, length);
        break;
      case CL_JSON_KEY_ICON_URL:
        cl_json_strcpy(ldb->icon_url, sizeof(ldb->icon_url), string, length);
        break;
      case CL_JSON_KEY_TITLE:
        cl_json_strcpy(ldb->title, sizeof(ldb->title), string, length);
        break;
      default:
        return 1;
      }
      break;
    }
    case CL_JSON_TYPE_MEMORY_NOTE:
    {
      cl_memnote_t *note = &((cl_memnote_t*)ud->data)[ud->element_num];

      switch (ud->field)
      {
      case CL_JSON_KEY_OFFSETS:
        if (!cl_json_parse_offsets(note, string))
          return 1;
        break;
#if CL_HAVE_EDITOR
      case CL_JSON_KEY_DESCRIPTION:
        cl_json_strcpy(note->details.description,
          sizeof(note->details.description),string, length);
        break;
      case CL_JSON_KEY_TITLE:
        cl_json_strcpy(note->details.title,
          sizeof(note->details.title), string, length);
        break;
#endif
      default:
        return 1;
      }
      break;
    }
    default:
      return 1;
    }
    ud->field = CL_JSON_KEY_NONE;
  }

  return 0;
}

bool cl_json_get(void *data, const char *json, cl_json_field key,
  cl_json_type type, unsigned size)
{
  const jsonsax_handlers_t handlers =
  {
    NULL,                /* start_document */
    NULL,                /* end_document   */
    NULL,                /* start_object   */
    NULL,                /* end_object     */
    cl_json_start_array, /* start_array    */
    cl_json_end_array,   /* end_array      */
    cl_json_key,         /* key            */
    NULL,                /* array_index    */
    cl_json_string,      /* string         */
    cl_json_number,      /* number         */
    cl_json_boolean,     /* boolean        */
    NULL                 /* null           */
  };
  cl_json_t value;

  memset(&value, 0, sizeof(value));
  value.data = data;
  value.element_count = 0;
  value.element_num = 0;
  value.is_current = false;
  value.field = CL_JSON_KEY_NONE;
  value.state = CL_JSON_STATE_STARTING;
  value.key = cl_json_key_name(key);
  value.size = size;
  value.type = type;

  if (jsonsax_parse(json, &handlers, (void*)&value) == JSONSAX_OK &&
      value.state == CL_JSON_STATE_FINISHED)
    return true;
  else
    return false;
}

bool cl_json_get_array(void **data, unsigned *elements, const char *json,
  cl_json_field key, cl_json_type type)
{
  /**
   * Do a first pass of the document to see how many elements are in the
   * requested array, so we can allocate only what we need
   */
  const jsonsax_handlers_t handlers_count =
  {
    NULL,                      /* start_document */
    NULL,                      /* end_document   */
    NULL,                      /* start_object   */
    NULL,                      /* end_object     */
    cl_json_start_array,       /* start_array    */
    cl_json_end_array,         /* end_array      */
    cl_json_key,               /* key            */
    cl_json_array_index_count, /* array_index    */
    NULL,                      /* string         */
    NULL,                      /* number         */
    NULL,                      /* boolean        */
    NULL                       /* null           */
  };
  const jsonsax_handlers_t handlers =
  {
    NULL,                  /* start_document */
    NULL,                  /* end_document   */
    NULL,                  /* start_object   */
    NULL,                  /* end_object     */
    cl_json_start_array,   /* start_array    */
    cl_json_end_array,     /* end_array      */
    cl_json_key_array,     /* key            */
    cl_json_array_index,   /* array_index    */
    cl_json_string_array,  /* string         */
    cl_json_number_array,  /* number         */
    cl_json_boolean_array, /* boolean        */
    NULL                   /* null           */
  };
  cl_json_t value;

  memset(&value, 0, sizeof(value));
  value.data = NULL;
  value.element_count = 0;
  value.element_num = 0;
  value.field = CL_JSON_KEY_NONE;
  value.is_current = false;
  value.key = cl_json_key_name(key);
  value.size = 0;
  value.state = CL_JSON_STATE_STARTING;
  value.type = type;

  /* Were we able to count the number of elements in the array? */
  if (jsonsax_parse(json, &handlers_count, (void*)&value) != JSONSAX_OK ||
                    value.element_count == 0)
    return false;

  /* Allocate memory for the array of given element type */
  *elements = value.element_count;
  switch (type)
  {
  case CL_JSON_TYPE_ACHIEVEMENT:
    *data = (cl_achievement_t*)calloc(value.element_count,
      sizeof(cl_achievement_t));
    break;
  case CL_JSON_TYPE_LEADERBOARD:
    *data = (cl_leaderboard_t*)calloc(value.element_count,
      sizeof(cl_leaderboard_t));
    break;
  case CL_JSON_TYPE_MEMORY_NOTE:
    *data = (cl_memnote_t*)calloc(value.element_count,
      sizeof(cl_memnote_t));
    break;
  default:
    return false;
  }

  /* Now, actually extract the values */
  memset(&value, 0, sizeof(value));
  value.data = *data;
  value.element_count = *elements;
  value.element_num = 0;
  value.field = CL_JSON_KEY_NONE;
  value.is_current = false;
  value.key = cl_json_key_name(key);
  value.size = 0;
  value.state = CL_JSON_STATE_STARTING;
  value.type = type;

  if (jsonsax_parse(json, &handlers, (void*)&value) != JSONSAX_OK ||
                    value.state != CL_JSON_STATE_FINISHED)
  {
    free(*data);
    return false;
  }
  else
    return true;
}
