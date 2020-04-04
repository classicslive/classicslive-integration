#ifndef CL_JSON_C
#define CL_JSON_C

#include <formats/jsonsax.h>
#include "cl_common.h"
#include "cl_json.h"

int cl_json_key(void *userdata, const char *name, size_t length)
{
   cl_json_t* ud = (cl_json_t*)userdata;

   ud->is_current = !strncmp(ud->key, name, length);

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
      case CL_JSON_BOOLEAN:
         *((bool*)ud->data) = istrue ? true : false;
         ud->found = true;
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

   if (ud->is_current)
   {
      ud->is_current = false;
      switch (ud->type)
      {
      case CL_JSON_NUMBER:
         ud->found = cl_strto(&number, ud->data, ud->size, false);
         break;
      default:
         return 1;
      }
   }

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
      case CL_JSON_BOOLEAN:
         if (string_is_equal(ud->data, "true"))
            *((bool*)ud->data) = true;
         else
            *((bool*)ud->data) = false;
         break;
      case CL_JSON_NUMBER:
         cl_strto(&string, ud->data, ud->size, false);
         break;
      case CL_JSON_STRING:
         snprintf(ud->data, length, "%s", string);
         ((char*)ud->data)[length] = '\0';
         break;
      default:
         return 1;
      }
      ud->found = true;
   }

   return 0;
}

bool cl_json_get(void *data, const char *json, const char *key, uint8_t type, uint32_t size)
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

   value.data       = data;
   value.is_current = false;
   value.found      = false;
   value.key        = key;
   value.size       = size;
   value.type       = type;

   if (jsonsax_parse(json, &handlers, (void*)&value) == JSONSAX_OK && value.found)
      return true;
   else
      return false;
}

#endif
