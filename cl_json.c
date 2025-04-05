#include <formats/jsonsax.h>
#include <string/stdstring.h>

#include "cl_common.h"
#include "cl_json.h"

int cl_json_key(void *userdata, const char *name, size_t length)
{
  cl_json_t* ud = (cl_json_t*)userdata;

  if (ud->state != CL_JSON_STATE_FINISHED)
  {
    ud->is_current = !strncmp(ud->key, name, length);
    if (ud->is_current)
      ud->state = CL_JSON_STATE_KEY_FOUND;
  }

  return 0;
}

int cl_json_boolean(void *userdata, int istrue)
{
  cl_json_t* ud = (cl_json_t*)userdata;

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

int cl_json_number(void *userdata, const char* number, size_t length)
{
  cl_json_t* ud = (cl_json_t*)userdata;
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

static int cl_json_start_array_count(void *userdata)
{
  cl_json_t* ud = (cl_json_t*)userdata;

  if (ud->is_current)
    ud->state = CL_JSON_STATE_ARRAY_STARTED;

  return 0;
}

static int cl_json_end_array_count(void *userdata)
{
  cl_json_t* ud = (cl_json_t*)userdata;

  if (ud->is_current)
  {
    ud->state = CL_JSON_STATE_FINISHED;
    ud->is_current = false;
  }

  return 0;
}

static int cl_json_array_index_count(void *userdata, unsigned index)
{
  cl_json_t* ud = (cl_json_t*)userdata;

  if (ud->is_current)
    ud->element_count = index + 1;

  return 0;
}

int cl_json_string(void *userdata, const char *string, size_t length)
{
  cl_json_t* ud = (cl_json_t*)userdata;

  if (ud->is_current)
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
      snprintf(ud->data, length + 1, "%s", string);
      ((char*)ud->data)[length] = '\0';
      break;
    default:
      return 1;
    }
    ud->state = CL_JSON_STATE_FINISHED;
  }

  return 0;
}

bool cl_json_get(void *data, const char *json, const char *key, unsigned type,
  unsigned size)
{
  const jsonsax_handlers_t handlers =
  {
    NULL,            /* start_document */
    NULL,            /* end_document   */
    NULL,            /* start_object   */
    NULL,            /* end_object     */
    NULL,            /* start_array    */
    NULL,            /* end_array      */
    cl_json_key,     /* key            */
    NULL,            /* array_index    */
    cl_json_string,  /* string         */
    cl_json_number,  /* number         */
    cl_json_boolean, /* boolean        */
    NULL             /* null           */
  };
  cl_json_t value;

  value.data = data;
  value.is_current = false;
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

bool cl_json_get_array(void *data, unsigned *elements, const char *json,
  const char *key, unsigned type, unsigned size)
{
#if 0
  const jsonsax_handlers_t handlers =
  {
    NULL,            /* start_document */
    NULL,            /* end_document   */
    NULL,            /* start_object   */
    NULL,            /* end_object     */
    cl_json_start_array, /* start_array    */
    NULL,            /* end_array      */
    cl_json_key,     /* key            */
    NULL,            /* array_index    */
    cl_json_string,  /* string         */
    cl_json_number,  /* number         */
    cl_json_boolean, /* boolean        */
    NULL             /* null           */
  };
  cl_json_t value;

  value.data = data;
  value.is_current = false;
  value.found = false;
  value.key = key;
  value.size = size;
  value.type = type;
#endif
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
    cl_json_start_array_count, /* start_array    */
    cl_json_end_array_count,   /* end_array      */
    cl_json_key,               /* key            */
    cl_json_array_index_count, /* array_index    */
    NULL,                      /* string         */
    NULL,                      /* number         */
    NULL,                      /* boolean        */
    NULL                       /* null           */
  };
  cl_json_t value;

  value.data = NULL;
  value.is_current = false;
  value.state = CL_JSON_STATE_STARTING;
  value.key = key;
  value.size = 0;
  value.type = type;

  /* Were we able to count the number of elements in the array? */
  if (jsonsax_parse(json, &handlers_count, (void*)&value) != JSONSAX_OK ||
                    value.state != CL_JSON_STATE_FINISHED)
    return false;

  /* Allocate memory for the array of given element type */
  *elements = value.element_count;
  switch (type)
  {
  case CL_JSON_TYPE_ACHIEVEMENT:
    data = (cl_achievement_t*)calloc(value.element_count, sizeof(cl_achievement_t));
    break;
  case CL_JSON_TYPE_LEADERBOARD:
    data = (cl_leaderboard_t*)calloc(value.element_count, sizeof(cl_leaderboard_t));
    break;
  default:
    return false;
  }
}
