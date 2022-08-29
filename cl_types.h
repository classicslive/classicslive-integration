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
  unsigned uintval;
} cl_arg_t;

typedef struct
{
   const char *data;
   unsigned    error_code;
   const char *error_msg;
} cl_network_response_t;

typedef void (*cl_network_cb_t)(cl_network_response_t);

typedef struct cl_session_t
{
   char     checksum[64];
   char     content_name[256];
   uint16_t game_id;
   char     game_name[256];
   char     generic_post[2048];
   bool     cheats_on;
   bool     states_on;
   char     id[CL_SESSION_ID_LENGTH];
   time_t   last_status_update;
   bool     ready;
} cl_session_t;

typedef struct cl_task_t
{
   void  *state;
   void (*handler)(struct cl_task_t*);
   void (*callback)(struct cl_task_t*);
   char  *error;
} cl_task_t;

typedef struct
{
  const char *username;
  const char *password;
  const char *token;
  const char *language;
} cl_user_t;

/* A virtual address for the emulated system. */
typedef uint64_t cl_addr_t;

#endif
