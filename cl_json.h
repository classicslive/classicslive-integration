#ifndef CL_JSON_H
#define CL_JSON_H

#define CL_JSON_KEY     1
#define CL_JSON_STRING  2
#define CL_JSON_NUMBER  3
#define CL_JSON_BOOLEAN 4

typedef struct cl_json_t
{
   void       *data;
   bool        is_current;
   bool        found;
   const char *key;
   uint32_t    size;
   uint8_t     type;
} cl_json_t;

bool cl_json_get(void *data, const char *json, const char *key, uint8_t type,
   uint32_t size);

#endif
