#include "cl_common.h"

#if CL_SHOW_ERRORS || CL_LOGGING
#include "cl_abi.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <stdint.h>

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
  cl_abi_display_message(level, msg);
  va_end(argv);
#endif
}

#ifdef __GNUC__
__attribute__((__format__ (__printf__, 1, 0)))
#endif
void cl_log(const char *format, ...)
{
#if CL_LOGGING
  if (!format)
    return;
  else
  {
    va_list argv;

    va_start(argv, format);
    vprintf(format, argv);
    va_end(argv);
  }
#endif
}

cl_value_type cl_pointer_type(const unsigned size)
{
  switch (size)
  {
  case 1:
    return CL_MEMTYPE_UINT8;
  case 2:
    return CL_MEMTYPE_UINT16;
  case 3:
  case 4:
    return CL_MEMTYPE_UINT32;
  case 5:
  case 6:
  case 7:
  case 8:
    return CL_MEMTYPE_INT64;
  }

  return CL_MEMTYPE_NOT_SET;
}

cl_error cl_read_8(void *value, const void *src, cl_addr_t offset)
{
#if CL_SAFETY
  if (!src || !value)
    return CL_ERR_PARAMETER_NULL;
#endif
  *(unsigned char*)value = ((unsigned char*)src)[offset];

  return CL_OK;
}

cl_error cl_read_16(void *value, const void *src, cl_addr_t offset,
  cl_endianness endianness)
{
#if CL_SAFETY
  if (!src || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint16_t tmp = *((const uint16_t*)(
    (const uint8_t*)src + offset));

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    *((uint16_t*)value) = _byteswap_ushort(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    *((uint16_t*)value) = __builtin_bswap16(tmp);
#else
    tmp = (tmp >> 8) | (tmp << 8);
#endif
  }
  *((uint16_t*)value) = tmp;

  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_read_32(void *value, const void *src, cl_addr_t offset,
                    cl_endianness endianness)
{
#if CL_SAFETY
  if (!src || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint32_t tmp = *((const uint32_t*)(
    (const uint8_t*)src + offset));

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    tmp = _byteswap_ulong(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    tmp = __builtin_bswap32(tmp);
#else
    tmp = ((tmp & 0x000000FFU) << 24) |
          ((tmp & 0x0000FF00U) << 8) |
          ((tmp & 0x00FF0000U) >> 8) |
          ((tmp & 0xFF000000U) >> 24);
#endif
  }
  *((uint32_t*)value) = tmp;
  
  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_read_64(void *value, const void *src, cl_addr_t offset,
                    cl_endianness endianness)
{
#if CL_SAFETY
  if (!src || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint64_t tmp = *((const uint64_t*)(
    (const uint8_t*)src + offset));

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    tmp = _byteswap_uint64(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    tmp = __builtin_bswap64(tmp);
#else
    tmp = ((tmp & 0x00000000000000FFULL) << 56) |
          ((tmp & 0x000000000000FF00ULL) << 40) |
          ((tmp & 0x0000000000FF0000ULL) << 24) |
          ((tmp & 0x00000000FF000000ULL) << 8)  |
          ((tmp & 0x000000FF00000000ULL) >> 8)  |
          ((tmp & 0x0000FF0000000000ULL) >> 24) |
          ((tmp & 0x00FF000000000000ULL) >> 40) |
          ((tmp & 0xFF00000000000000ULL) >> 56);
#endif
  }
  *((uint64_t*)value) = tmp;

  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_read_value(void *value, const void *src, cl_addr_t offset,
  cl_value_type type, cl_endianness endianness)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
  case CL_MEMTYPE_UINT8:
    return cl_read_8(value, src, offset);
  case CL_MEMTYPE_INT16:
  case CL_MEMTYPE_UINT16:
    return cl_read_16(value, src, offset, endianness);
  case CL_MEMTYPE_INT32:
  case CL_MEMTYPE_UINT32:
  case CL_MEMTYPE_FLOAT:
    return cl_read_32(value, src, offset, endianness);
  case CL_MEMTYPE_INT64:
  case CL_MEMTYPE_DOUBLE:
    return cl_read_64(value, src, offset, endianness);
  case CL_MEMTYPE_NOT_SET:
  case CL_MEMTYPE_SIZE:
    /* No default because we want warnings here */
    return CL_ERR_PARAMETER_INVALID;
  }

  return CL_ERR_PARAMETER_INVALID;
}

cl_error cl_read_buffer(void *dst, const void *src, cl_addr_t offset,
  cl_addr_t size)
{
#if CL_SAFETY
  if (!src || !dst)
    return CL_ERR_PARAMETER_NULL;
#endif
  memcpy(dst, (const unsigned char*)src + offset, size);

  return CL_OK;
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

cl_error cl_write_8(const void *value, void *dst, cl_addr_t offset)
{
#if CL_SAFETY
  if (!dst || !value)
    return CL_ERR_PARAMETER_NULL;
#endif
  ((uint8_t*)dst)[offset] = *(const uint8_t*)value;

  return CL_OK;
}

cl_error cl_write_16(const void *value, void *dst, cl_addr_t offset,
                     cl_endianness endianness)
{
#if CL_SAFETY
  if (!dst || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint16_t tmp = *(const uint16_t*)value;

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    tmp = _byteswap_ushort(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    tmp = __builtin_bswap16(tmp);
#else
    tmp = (tmp >> 8) | (tmp << 8);
#endif
  }

  *((uint16_t*)((uint8_t*)dst + offset)) = tmp;

  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_write_32(const void *value, void *dst, cl_addr_t offset,
                     cl_endianness endianness)
{
#if CL_SAFETY
  if (!dst || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint32_t tmp = *(const uint32_t*)value;

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    tmp = _byteswap_ulong(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    tmp = __builtin_bswap32(tmp);
#else
    tmp = ((tmp & 0x000000FFU) << 24) |
          ((tmp & 0x0000FF00U) << 8) |
          ((tmp & 0x00FF0000U) >> 8) |
          ((tmp & 0xFF000000U) >> 24);
#endif
  }

  *((uint32_t*)((uint8_t*)dst + offset)) = tmp;

  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_write_64(const void *value, void *dst, cl_addr_t offset,
                     cl_endianness endianness)
{
#if CL_SAFETY
  if (!dst || !value)
    return CL_ERR_PARAMETER_NULL;
  else
  {
#endif
  uint64_t tmp = *(const uint64_t*)value;

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  if (endianness == CL_ENDIAN_LITTLE)
#else
  if (endianness == CL_ENDIAN_BIG)
#endif
  {
#if defined(_MSC_VER)
    tmp = _byteswap_uint64(tmp);
#elif defined(__GNUC__) || defined(__clang__)
    tmp = __builtin_bswap64(tmp);
#else
    tmp = ((tmp & 0x00000000000000FFULL) << 56) |
          ((tmp & 0x000000000000FF00ULL) << 40) |
          ((tmp & 0x0000000000FF0000ULL) << 24) |
          ((tmp & 0x00000000FF000000ULL) << 8)  |
          ((tmp & 0x000000FF00000000ULL) >> 8)  |
          ((tmp & 0x0000FF0000000000ULL) >> 24) |
          ((tmp & 0x00FF000000000000ULL) >> 40) |
          ((tmp & 0xFF00000000000000ULL) >> 56);
#endif
  }

  *((uint64_t*)((uint8_t*)dst + offset)) = tmp;

  return CL_OK;
#if CL_SAFETY
  }
#endif
}

cl_error cl_write_value(const void *value, void *dst, cl_addr_t offset,
                        cl_value_type type, cl_endianness endianness)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
  case CL_MEMTYPE_UINT8:
    return cl_write_8(value, dst, offset);
  case CL_MEMTYPE_INT16:
  case CL_MEMTYPE_UINT16:
    return cl_write_16(value, dst, offset, endianness);
  case CL_MEMTYPE_INT32:
  case CL_MEMTYPE_UINT32:
  case CL_MEMTYPE_FLOAT:
    return cl_write_32(value, dst, offset, endianness);
  case CL_MEMTYPE_INT64:
  case CL_MEMTYPE_DOUBLE:
    return cl_write_64(value, dst, offset, endianness);
  case CL_MEMTYPE_NOT_SET:
  case CL_MEMTYPE_SIZE:
    /* No default because we want warnings here */
    return CL_ERR_PARAMETER_INVALID;
  }

  return CL_ERR_PARAMETER_INVALID;
}

cl_error cl_write_buffer(const void *src, void *dst, cl_addr_t offset,
  cl_addr_t size)
{
#if CL_SAFETY
  if (!dst || !src)
    return CL_ERR_PARAMETER_NULL;
#endif
  memcpy((unsigned char*)dst + offset, src, size);

  return CL_OK;
}

unsigned cl_sizeof_memtype(const cl_value_type type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
  case CL_MEMTYPE_UINT8:
    return 1;
  case CL_MEMTYPE_INT16:
  case CL_MEMTYPE_UINT16:
    return 2;
  case CL_MEMTYPE_INT32:
  case CL_MEMTYPE_UINT32:
  case CL_MEMTYPE_FLOAT:
    return 4;
  case CL_MEMTYPE_INT64:
  case CL_MEMTYPE_DOUBLE:
    return 8;
  default:
    /* Should not be reached */
    cl_message(CL_MSG_ERROR, "cl_sizeof_memtype bad value %u", type);
    return 0;
  }
}

const char *cl_string_bitness(cl_bitness bitness)
{
  switch (bitness)
  {
  case CL_BITNESS_32:
    return "32-bit";
  case CL_BITNESS_64:
    return "64-bit";
  case CL_BITNESS_UNKNOWN:
  case CL_BITNESS_SIZE:
    return "Invalid bitness";
  }

  return "Undefined bitness";
}

const char *cl_string_endianness(cl_endianness endianness)
{
  switch (endianness)
  {
  case CL_ENDIAN_LITTLE:
    return "Little-endian";
  case CL_ENDIAN_BIG:
    return "Big-endian";
  case CL_ENDIAN_WORD_FLIP_BL:
    return "Word-flip big-little-endian";
  case CL_ENDIAN_WORD_FLIP_LB:
    return "Word-flip little-big-endian";
  case CL_ENDIAN_INVALID:
    return "Invalid endianness";
  }

  return "Undefined endianness";
}

const char *cl_string_platform(cl_platform platform)
{
  switch (platform)
  {
  case CL_PLATFORM_WINDOWS:
    return "Windows";
  case CL_PLATFORM_LINUX:
    return "Linux";
  case CL_PLATFORM_MACOS:
    return "macOS";
  case CL_PLATFORM_ANDROID:
    return "Android";
  case CL_PLATFORM_NINTENDO_64:
    return "Nintendo 64";
  case CL_PLATFORM_GAMECUBE:
    return "GameCube";
  case CL_PLATFORM_WII:
    return "Wii";
  case CL_PLATFORM_WII_U:
    return "Wii U";
  case CL_PLATFORM_SWITCH:
    return "Switch";
  case CL_PLATFORM_SWITCH_2:
    return "Switch 2";
  case CL_PLATFORM_UNKNOWN:
  case CL_PLATFORM_SIZE:
    return "Invalid platform";
  }

  return "Undefined platform";
}

const char *cl_string_error(cl_error error)
{
  switch (error)
  {
  case CL_OK:
    return "No error";
  case CL_ERR_UNKNOWN:
    return "Unknown error";
  case CL_ERR_USER_CONFIG:
    return "User configuration error";
  case CL_ERR_CLIENT_RUNTIME:
    return "Client runtime error";
  case CL_ERR_CLIENT_COMPILE:
    return "Client compile error";
  case CL_ERR_SERVER_NOT_FOUND:
    return "Server not found";
  case CL_ERR_SERVER_UNAVAILABLE:
    return "Server unavailable";
  case CL_ERR_SERVER_INTERNAL:
    return "Server internal error";
  case CL_ERR_SERVER_UNEXPECTED_RESPONSE:
    return "Server unexpected response";
  case CL_ERR_PARAMETER_INVALID:
    return "Invalid parameter";
  case CL_ERR_PARAMETER_NULL:
    return "Null parameter";
  case CL_ERR_SESSION_MISMATCH:
    return "Session mismatch";
  case CL_ERR_SIZE:
    return "Error size limit reached";
  }

  return "Invalid error code";
}
