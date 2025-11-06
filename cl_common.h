#ifndef CL_COMMON_H
#define CL_COMMON_H

#include "cl_types.h"

/**
 * A -1 value to represent invalid addresses in memory regions, as 0 for NULL
 * may be a valid address on some emulated systems.
 */
#define CL_ADDRESS_INVALID (void*)0xFFFFFFFFFFFFFFFF

/**
 * Maximum size, in bytes, to use when generating an MD5 hash of raw content
 * data.
 */
#define CL_CONTENT_SIZE_LIMIT 256 * 1024 * 1024

#define CL_GLOBALS_SIZE 3 * MAX_USERS

/**
 * Magic number representing the integration version. Server output may include
 * a minimum number so requests can be cancelled if the local integration is
 * not a recent enough build.
 */
#define CL_INTEGRATION_VERSION 1

#define CL_LOGGING true

/**
 * How often, in seconds, to ping back to the server to update current status.
 */
#define CL_PRESENCE_INTERVAL 60

/**
 * Radix used in output retrieved from the server.
 */
#define CL_RADIX 10

#define CL_SHOW_ERRORS true

/**
 * Format string to print the contents of a buffer representing an MD5 hash.
 */
#define CL_SNPRINTF_MD5 "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"

#define CL_TASK_MUTE true

/**
 * Used for unit tests to print a descriptive failure message.
 */
#define CL_TEST_FAIL(a) { printf("Test failed in %s on line %u.", \
  __FILE__, \
  __LINE__); \
  exit(a); };

typedef enum
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
} cl_endianness;

typedef enum
{
  CL_MSG_DEBUG = 0,

  CL_MSG_INFO,
  CL_MSG_WARN,
  CL_MSG_ERROR,

  CL_MSG_SIZE
} cl_log_level;

typedef enum
{
  CL_MEMTYPE_NOT_SET = 0,

  CL_MEMTYPE_INT64,
  CL_MEMTYPE_DOUBLE,

  CL_MEMTYPE_INT8,
  CL_MEMTYPE_UINT8,
  CL_MEMTYPE_INT16,
  CL_MEMTYPE_UINT16,
  CL_MEMTYPE_INT32,
  CL_MEMTYPE_UINT32,
  CL_MEMTYPE_FLOAT,

  CL_MEMTYPE_SIZE
} cl_value_type;

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
