#ifndef CL_JSON_H
#define CL_JSON_H

typedef enum
{
  CL_JSON_TYPE_NONE = 0,

  CL_JSON_TYPE_KEY,
  CL_JSON_TYPE_STRING,
  CL_JSON_TYPE_NUMBER,
  CL_JSON_TYPE_BOOLEAN,

  CL_JSON_TYPE_ACHIEVEMENT,
  CL_JSON_TYPE_LEADERBOARD,

  CL_JSON_TYPE_SIZE
} cl_json_type;

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

  /** Whether or not the requested element was copied into the buffer */
  cl_json_state state;

  /** The size, in bytes, of the buffer to be written into */
  unsigned size;

  /** The type of JSON data we are looking for */
  cl_json_type type;

  /** In array decoding, the index of the current element */
  unsigned element_num;

  /** In array decoding, the total number of elements */
  unsigned element_count;
} cl_json_t;

bool cl_json_get(void *data, const char *json, const char *key, unsigned type,
  unsigned size);

bool cl_json_get_array(void *data, unsigned *elements, const char *json,
  const char *key, unsigned type, unsigned size);

#endif
