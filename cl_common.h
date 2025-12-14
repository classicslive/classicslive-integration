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
 * Reads data from one location to another, automatically applying transforms
 *   for size and endianness differences.
 * @param dest The destination buffer.
 * @param src The source buffer.
 * @param offset The location from which to start reading from src.
 * @param size The number of bytes to read from src.
 * @param endianness The endianness of dest. For example, CL_ENDIAN_LITTLE.
 * @return Whether the read succeeded.
 */
bool cl_read(void *dest, const uint8_t *src, cl_addr_t offset, unsigned size,
  cl_endianness endianness);

/**
 * Writes data from one location to another, automatically applying transforms
 *   for size and endianness differences.
 * @param dest The destination buffer.
 * @param src The source buffer.
 * @param offset The location from which to start reading from src.
 * @param size The number of bytes to read from src.
 * @param endianness The endianness of dest. For example, CL_ENDIAN_LITTLE.
 * @return Whether the write succeeded.
 */
bool cl_write(uint8_t *dest, const void *src, cl_addr_t offset, unsigned size,
  cl_endianness endianness);

bool cl_strto(const char **pos, void *value, unsigned size, bool is_signed);

#endif
