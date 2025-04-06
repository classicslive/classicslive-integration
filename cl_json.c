#include <formats/jsonsax.h>
#include <string/stdstring.h>

#include "cl_common.h"
#include "cl_json.h"

static int cl_json_key(void *userdata, const char *name, size_t length)
{
  cl_json_t *ud = (cl_json_t*)userdata;

  if (ud->state != CL_JSON_STATE_FINISHED && !ud->array_level && !ud->is_object)
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
  {
    if (!strncmp(name, "title", length))
      ud->field = CL_JSON_KEY_TITLE;
    else if (!strncmp(name, "description", length))
      ud->field = CL_JSON_KEY_DESCRIPTION;
    else if (!strncmp(name, "details", length))
      ud->field = CL_JSON_KEY_DETAILS;
    else if (!strncmp(name, "icon_url", length))
      ud->field = CL_JSON_KEY_ICON_URL;
    else if (!strncmp(name, "flags", length))
      ud->field = CL_JSON_KEY_FLAGS;
    else if (!strncmp(name, "id", length))
      ud->field = CL_JSON_KEY_ID;
    else if (!strncmp(name, "unlocked", length))
      ud->field = CL_JSON_KEY_UNLOCKED;
    else
      ud->field = CL_JSON_KEY_NONE;
  }
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
        *((bool*)ach->unlocked) = istrue ? true : false;
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

static int cl_json_number_array(void *userdata, const char* number, size_t length)
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

static void cl_json_strcpy(char *dst, size_t dstlen, const char *src,
  size_t srclen)
{
  snprintf(dst, dstlen, "%s", src);
  dst[srclen] = '\0';
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

static int cl_json_string_array(void *userdata, const char *string, size_t length)
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
        cl_json_strcpy(ach->description, sizeof(ach->description), string, length);
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
        cl_json_strcpy(ldb->description, sizeof(ldb->description), string, length);
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
    default:
      return 1;
    }
    ud->field = CL_JSON_KEY_NONE;
  }

  return 0;
}

bool cl_json_get(void *data, const char *json, const char *key, unsigned type,
  unsigned size)
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

  value.data = data;
  value.element_count = 0;
  value.element_num = 0;
  value.is_current = false;
  value.field = CL_JSON_KEY_NONE;
  value.state = CL_JSON_STATE_STARTING;
  value.key = key;
  value.size = size;
  value.type = type;

  if (jsonsax_parse(json, &handlers, (void*)&value) == JSONSAX_OK &&
      value.state == CL_JSON_STATE_FINISHED)
    return true;
  else
    return false;
}

bool cl_json_get_array(void **data, unsigned *elements, const char *json,
  const char *key, unsigned type)
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
  value.key = key;
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
  value.key = key;
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
