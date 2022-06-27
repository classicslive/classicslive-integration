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

#define CL_ENDIAN_LITTLE       0
#define CL_ENDIAN_BIG          1

enum
{
  CL_MSG_DEBUG = 0,

  CL_MSG_INFO,
  CL_MSG_WARN,
  CL_MSG_ERROR,

  CL_MSG_SIZE
};

void cl_message(unsigned level, const char *format, ...);
void cl_log    (const char *format, ...);
bool cl_read   (void *dest, const uint8_t *src, cl_addr_t offset, unsigned size, unsigned endianness);
bool cl_write  (uint8_t *dest, const void *src, cl_addr_t offset, uint8_t size, uint8_t endianness);
bool cl_strto  (const char **pos, void *value, uint8_t size, bool is_signed);

#endif
