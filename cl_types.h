#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CL_SESSION_ID_LENGTH 32

typedef union
{
  int64_t intval;
  double floatval;
  uint64_t uintval;
} cl_arg_t;

typedef struct
{
   const char *data;
   unsigned    error_code;
   const char *error_msg;
} cl_network_response_t;

typedef void (*cl_network_cb_t)(cl_network_response_t);

typedef union cl_session_flags_t
{
  unsigned intval;
  struct
  {
    unsigned cheats : 1;
    unsigned states : 1;
    unsigned fast_forward : 1;
    unsigned pause : 1;
  };
} cl_session_flags_t;

typedef struct cl_session_t
{
   char     checksum[64];
   char     content_name[256];
   unsigned game_id;
   char     game_name[256];
   char     generic_post[2048];
   cl_session_flags_t flags;
   char     id[CL_SESSION_ID_LENGTH];
   time_t   last_status_update;
   bool     ready;
} cl_session_t;

struct cl_task_t;

typedef void (*CL_TASK_CB_T)(struct cl_task_t*);

typedef struct cl_task_t
{
   void  *state;
   CL_TASK_CB_T handler;
   CL_TASK_CB_T callback;
   char  *error;
} cl_task_t;

typedef struct
{
  char username[64];
  char password[64];
  char token[32];
  char language[8];
} cl_user_t;

/** A virtual address for the emulated system. */
typedef uintptr_t cl_addr_t;

#endif
