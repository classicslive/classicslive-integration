#ifndef CL_COMMON_H
#define CL_COMMON_H

#include "cl_types.h"

#define CL_CONTENT_SIZE_LIMIT  256 * 1024 * 1024
#define CL_GLOBALS_SIZE        3 * MAX_USERS
#define CL_INTEGRATION_VERSION 1
#define CL_LOGGING             true
#define CL_PRESENCE_INTERVAL   60
#define CL_RADIX               16
#define CL_SHOW_ERRORS         true
#define CL_SNPRINTF_MD5        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
#define CL_TASK_MUTE           true

enum
{
  /* EFCDAB8967452301 */
  CL_ENDIAN_LITTLE = 0,

  /* 0123456789ABCDEF */
  CL_ENDIAN_BIG,

  /* 67452301EFCDAB89 */
  CL_ENDIAN_WORD_FLIP_BL,

  /* 89ABCDEF01234567 */
  CL_ENDIAN_WORD_FLIP_LB,

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  CL_ENDIAN_NATIVE = CL_ENDIAN_BIG,
#else
  CL_ENDIAN_NATIVE = CL_ENDIAN_LITTLE,
#endif

  CL_ENDIAN_SIZE
};

#define CL_UNUSED(a) (void)(a)

enum
{
  CL_MSG_DEBUG = 0,

  CL_MSG_INFO,
  CL_MSG_WARN,
  CL_MSG_ERROR,

  CL_MSG_SIZE
};

/**
 * Formats a message and instructs the frontend to display or log it.
 * @param level The severity of the message. For example, CL_MSG_ERROR.
 * @param format A printf format string.
 **/
void cl_message(unsigned level, const char *format, ...);

/**
 * Formats a message and logs it. Line breaks must be manually applied.
 * @param format A printf format string.
 **/
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
 **/
bool cl_read(void *dest, const uint8_t *src, cl_addr_t offset, unsigned size,
  unsigned endianness);

/**
 * Writes data from one location to another, automatically applying transforms
 *   for size and endianness differences.
 * @param dest The destination buffer.
 * @param src The source buffer.
 * @param offset The location from which to start reading from src.
 * @param size The number of bytes to read from src.
 * @param endianness The endianness of dest. For example, CL_ENDIAN_LITTLE.
 * @return Whether the write succeeded.
 **/
bool cl_write(uint8_t *dest, const void *src, cl_addr_t offset, unsigned size,
  unsigned endianness);

bool cl_strto(const char **pos, void *value, unsigned size, bool is_signed);

#endif
