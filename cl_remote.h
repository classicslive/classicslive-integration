#ifndef CL_REMOTE_H
#define CL_REMOTE_H

/**
 * @file cl_remote.h
 * A small TCP protocol for accessing the memory of software running on real
 * hardware from a frontend on another machine, ie. to use memory searching
 * and the Live Editor against a console.
 *
 * Every message, in either direction, is a `cl_remote_header_t` followed by
 * `size` bytes of payload. Responses use the same command as their request.
 * All values are little-endian.
 */

#include <stdint.h>

#define CL_REMOTE_PORT 17228

/* "CLRM" */
#define CL_REMOTE_MAGIC 0x4D524C43

#define CL_REMOTE_VERSION 1

/* The largest READ the server will answer, matching the default search bucket */
#define CL_REMOTE_MAX_READ (128 * 1024 * 1024)

typedef enum
{
  CL_REMOTE_CMD_INVALID = 0,

  /**
   * Request: no payload.
   * Response: `cl_remote_hello_t`.
   */
  CL_REMOTE_CMD_HELLO,

  /**
   * Request: no payload.
   * Response: a `uint32_t` count, then that many `cl_remote_region_t`.
   */
  CL_REMOTE_CMD_REGIONS,

  /**
   * Request: `cl_remote_read_t`.
   * Response: exactly `size` bytes. Bytes that could not be read are zero.
   */
  CL_REMOTE_CMD_READ,

  /**
   * Request: a `uint64_t` address, then the bytes to write.
   * Response: no payload.
   */
  CL_REMOTE_CMD_WRITE,

  /**
   * Request: no payload.
   * Response: no payload. The server resumes the software if the client
   *   disconnects while it is paused.
   */
  CL_REMOTE_CMD_PAUSE,
  CL_REMOTE_CMD_RESUME,

  CL_REMOTE_CMD_SIZE
} cl_remote_command;

typedef enum
{
  CL_REMOTE_OK = 0,

  /* The request was malformed or too large */
  CL_REMOTE_ERR_REQUEST,

  /* No software is running, or it could not be attached to */
  CL_REMOTE_ERR_NO_PROCESS,

  /* The memory access or operation failed */
  CL_REMOTE_ERR_FAILED,

  CL_REMOTE_ERR_SIZE
} cl_remote_status;

#pragma pack(push, 1)

typedef struct
{
  uint32_t magic;
  uint16_t command;
  uint16_t status;
  uint64_t size;
} cl_remote_header_t;

typedef struct
{
  uint32_t version;

  /* Nonzero if software is running and attached */
  uint32_t attached;

  uint64_t program_id;
  uint64_t process_id;
  uint8_t build_id[32];
  char display_version[16];
  char title[256];
} cl_remote_hello_t;

typedef struct
{
  uint64_t base;
  uint64_t size;

  /* Platform-specific; on Switch, the MemoryType and Permission of the map */
  uint32_t type;
  uint32_t permission;
} cl_remote_region_t;

typedef struct
{
  uint64_t address;
  uint64_t size;
} cl_remote_read_t;

#pragma pack(pop)

#endif
