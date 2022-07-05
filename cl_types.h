#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CL_SESSION_ID_LENGTH 32

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

/* A virtual address for the emulated system. */
typedef uint64_t cl_addr_t;

#endif
