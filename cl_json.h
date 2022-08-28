#ifndef CL_JSON_H
#define CL_JSON_H

enum
{
  CL_JSON_KEY = 0,

  CL_JSON_STRING,
  CL_JSON_NUMBER,
  CL_JSON_BOOLEAN,

  CL_JSON_SIZE
};

typedef struct cl_json_t
{
  void       *data;
  bool        is_current;
  bool        found;
  const char *key;
  unsigned    size;
  unsigned    type;
} cl_json_t;

bool cl_json_get(void *data, const char *json, const char *key, unsigned type,
  unsigned size);

#endif
