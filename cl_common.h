#ifndef CL_COMMON_H
#define CL_COMMON_H

#include "cl_types.h"

/**
 * Used to explicitly ignore unused variables to prevent warnings.
 */
#define CL_UNUSED(a) (void)(a)

/**
 * Formats a message and instructs the frontend to display or log it.
 * @param level The severity of the message. For example, CL_MSG_ERROR.
 * @param format A printf format string.
 */
void cl_message(cl_log_level level, const char *format, ...);

/**
 * Formats a message and logs it. Line breaks must be manually applied.
 * @param format A printf format string.
 */
void cl_log(const char *format, ...);

/**
 * Returns the value type that represents a pointer of a given size.
 * @param size Pointer size, in bytes
 */
cl_value_type cl_pointer_type(const unsigned size);

cl_error cl_read_8(void *value, const void *src, cl_addr_t offset);
cl_error cl_read_16(void *value, const void *src, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_read_32(void *value, const void *src, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_read_64(void *value, const void *src, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_read_value(void *value, const void *src, cl_addr_t offset,
  cl_value_type type, cl_endianness endianness);
cl_error cl_read_buffer(void *dst, const void *src, cl_addr_t offset,
  cl_addr_t size);

cl_error cl_write_8(const void *value, void *dst, cl_addr_t offset);
cl_error cl_write_16(const void *value, void *dst, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_write_32(const void *value, void *dst, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_write_64(const void *value, void *dst, cl_addr_t offset,
  cl_endianness endianness);
cl_error cl_write_value(const void *value, void *dst, cl_addr_t offset,
  cl_value_type type, cl_endianness endianness);
cl_error cl_write_buffer(const void *src, void *dst, cl_addr_t offset,
  cl_addr_t size);

/**
 * Returns the size (in bytes) of a given value type.
 * @param type A type of memory value. For example, CL_MEMTYPE_8BIT.
 * @return The number of bytes the memory type takes up, or 0 if invalid.
 */
unsigned cl_sizeof_memtype(const cl_value_type type);

const char *cl_string_bitness(cl_bitness bitness);
const char *cl_string_endianness(cl_endianness endianness);
const char *cl_string_error(cl_error error);
const char *cl_string_platform(cl_platform platform);

bool cl_strto(const char **pos, void *value, unsigned size, bool is_signed);

#endif
