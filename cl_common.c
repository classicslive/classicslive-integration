#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "cl_common.h"
#include "cl_frontend.h"

#ifdef __GNUC__
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void cl_message(cl_log_level level, const char *format, ...)
{
#if CL_SHOW_ERRORS
  char msg[256];
  va_list argv;

  va_start(argv, format);
  vsprintf(msg, format, argv);
  msg[sizeof(msg) - 1] = '\0';
  cl_fe_display_message(level, msg);
  va_end(argv);
#endif
}

#ifdef __GNUC__
__attribute__((__format__ (__printf__, 1, 0)))
#endif
void cl_log(const char *format, ...)
{
#if CL_LOGGING
  va_list argv;

  va_start(argv, format);
  vprintf(format, argv);
  va_end(argv);
#endif
}

bool cl_read(void *dest, const uint8_t *src, cl_addr_t offset, unsigned size, 
   unsigned endianness)
{
  if (src && size > 0)
  {
    memcpy(dest, &src[offset], size);

    /* Byte swap if necessary */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    if (endianness == CL_ENDIAN_LITTLE)
#else
    if (endianness == CL_ENDIAN_BIG)
#endif
    {
      switch (size)
      {
      case 2:
#ifdef _MSC_VER
        *((uint16_t*)dest) = _byteswap_ushort(*((uint16_t*)dest));
#else
        *((uint16_t*)dest) = __builtin_bswap16(*((uint16_t*)dest));
#endif
        break;
      case 4:
#ifdef _MSC_VER
        *((uint32_t*)dest) = _byteswap_ulong(*((uint32_t*)dest));
#else
        *((uint32_t*)dest) = __builtin_bswap32(*((uint32_t*)dest));
#endif
        break;
      case 8:
#ifdef _MSC_VER
        *((uint64_t*)dest) = _byteswap_uint64(*((uint64_t*)dest));
#else
        *((uint64_t*)dest) = __builtin_bswap64(*((uint64_t*)dest));
#endif
        break;
      }
    }

    return true;
  }

  return false;
}

bool cl_strto(const char **pos, void *value, unsigned size, bool is_signed)
{
  char *end = NULL;

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
  case 8:
    if (is_signed)
      *(int64_t*) value = (int64_t) strtoll (*pos, &end, CL_RADIX);
    else
      *(uint64_t*)value = (uint64_t)strtoull(*pos, &end, CL_RADIX);
    break;
  default:
    return false;
  }
  *pos = end;

  return true;
}

bool cl_write(uint8_t *dest, const void *src, cl_addr_t offset, unsigned size,
  unsigned endianness)
{
  unsigned i;

  if (dest && src)
  {
    if (endianness == CL_ENDIAN_LITTLE)
      memcpy(&dest[offset], src, size);
    else if (endianness == CL_ENDIAN_BIG)
      for (i = 0; i < size; i++)
        dest[offset + i] = ((const uint8_t*)src)[size - i - 1];
    else
      return false;
  }

  return false;
}
