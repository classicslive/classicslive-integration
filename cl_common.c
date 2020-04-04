#ifndef CL_COMMON_C
#define CL_COMMON_C

#include "cl_common.h"

void cl_error(const char *format, ...)
{
#ifndef CL_SHOW_ERRORS
   return;
#else
   char msg[256];
   va_list argv;

   va_start(argv, format);
   vsprintf(msg, format, argv);
   msg[sizeof(msg) - 1] = '\0';
   runloop_msg_queue_push(msg, 0, 2 * 60, false, NULL, 0, 0);
   va_end(argv);
#endif
}

#define CL_LOGGING true
void cl_log(const char *format, ...)
{
#ifdef CL_LOGGING
   va_list argv;

   va_start(argv, format);
   vprintf(format, argv);
   va_end(argv);
#endif
}

bool cl_read(uint32_t *dest, const uint8_t *src, uint32_t offset, 
   uint8_t size, uint8_t endianness)
{
   if (src && size > 0)
   {
      *dest = 0;
      memcpy(dest, &src[offset], size);

      /* Byte swap if necessary */
#ifdef MSB_FIRST
      if (endianness == CL_ENDIAN_LITTLE)
         *dest = __builtin_bswap32(*dest) << ((4 - size) * 8);
#else
      if (endianness == CL_ENDIAN_BIG)
         *dest = __builtin_bswap32(*dest) >> ((4 - size) * 8);
#endif

      return true;
   }

   return false;
}

bool cl_strto(const char **pos, void *value, uint8_t size, bool is_signed)
{
   char* end = NULL;

   if (**pos == '\0')
      return false;

   switch (size)
   {
   case 1:
      if (is_signed)
         *(int8_t*)  value = (int8_t)  strtol (*pos, &end, CL_RADIX);
      else
         *(uint8_t*) value = (uint8_t) strtoul(*pos, &end, CL_RADIX);
      break;
   case 2:
      if (is_signed)
         *(int16_t*) value = (int16_t) strtol (*pos, &end, CL_RADIX);
      else
         *(uint16_t*)value = (uint16_t)strtoul(*pos, &end, CL_RADIX);
      break;
   case 4:
      if (is_signed)
         *(int32_t*) value = (int32_t) strtol (*pos, &end, CL_RADIX);
      else
         *(uint32_t*)value = (uint32_t)strtoul(*pos, &end, CL_RADIX);
      break;
   default:
      return false;
   }
   *pos = end;

   return true;
}

bool cl_write(uint8_t *dest, const void *src, uint32_t offset, uint8_t size, uint8_t endianness)
{
   uint8_t i;

   if (dest && src)
   {
      if (endianness == CL_ENDIAN_LITTLE)
         memcpy(&dest[offset], src, size);
      else if (endianness == CL_ENDIAN_BIG)
         for (i = 0; i < size; i++)
            dest[offset + i] = ((uint8_t*)src)[size - i - 1];
      else
         return false;
   }

   return false;
}

#endif