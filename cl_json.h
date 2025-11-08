#ifndef CL_JSON_H
#define CL_JSON_H

typedef enum
{
  CL_JSON_KEY_NONE = 0,

  CL_JSON_KEY_ACHIEVEMENTS,
  CL_JSON_KEY_ADDRESS,
  CL_JSON_KEY_DESCRIPTION,
  CL_JSON_KEY_DETAILS,
  CL_JSON_KEY_ENDIANNESS,
  CL_JSON_KEY_FLAGS,
  CL_JSON_KEY_GAME_ID,
  CL_JSON_KEY_ICON_URL,
  CL_JSON_KEY_ID,
  CL_JSON_KEY_LEADERBOARDS,
  CL_JSON_KEY_MEMORY_NOTES,
  CL_JSON_KEY_MEMORY_NOTE_ID,
  CL_JSON_KEY_OFFSETS,
  CL_JSON_KEY_ORDER,
  CL_JSON_KEY_POINTER_SIZE,
  CL_JSON_KEY_REASON,
  CL_JSON_KEY_SCRIPT,
  CL_JSON_KEY_SESSION_ID,
  CL_JSON_KEY_SUCCESS,
  CL_JSON_KEY_TITLE,
  CL_JSON_KEY_TYPE,
  CL_JSON_KEY_UNLOCKED,

  CL_JSON_KEY_SIZE
} cl_json_field;

typedef enum
{
  CL_JSON_TYPE_NONE = 0,

  CL_JSON_TYPE_KEY,
  CL_JSON_TYPE_STRING,
  CL_JSON_TYPE_NUMBER,
  CL_JSON_TYPE_BOOLEAN,

  CL_JSON_TYPE_ACHIEVEMENT,
  CL_JSON_TYPE_LEADERBOARD,
  CL_JSON_TYPE_MEMORY_NOTE,

  CL_JSON_TYPE_SIZE
} cl_json_type;

/**
 * @brief Extracts a single value from a JSON document.
 * @param data A pointer to a buffer that will be filled with the value.
 * @param json The JSON text to parse.
 * @param key The key of the value to extract.
 * @param type The type of the value.
 * @param size The size, in bytes, of the buffer to be written into.
 * @return true if the value was successfully extracted; false otherwise.
 */
bool cl_json_get(void *data, const char *json, cl_json_field key,
  cl_json_type type, unsigned size);

/**
 * @brief Extracts an array of elements from a JSON document.
 * @param data A pointer to a buffer that will be allocated to hold the array.
 *   The caller is responsible for freeing this buffer.
 * @param elements A pointer to an unsigned that will be filled with the number
 *   of elements in the array.
 * @param json The JSON text to parse.
 * @param key The key of the array to extract.
 * @param type The type of elements in the array.
 * @return true if the array was successfully extracted; false otherwise.
 */
bool cl_json_get_array(void **data, unsigned *elements, const char *json,
  cl_json_field key, cl_json_type type);

#endif
