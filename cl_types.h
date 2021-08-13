#ifndef CL_TYPES_H
#define CL_TYPES_H

#include "cl_common.h"

/* ============================================================================
   Miscellaneous info about the console being emulated, which may make 
   scripts process differently.
============================================================================ */
typedef struct cl_system_t
{
   uint8_t endianness;
   uint8_t pointer_size;
} cl_system_t;

typedef struct cl_session_t
{
   char     checksum[32 + 1];
   char     content_name[256];
   uint16_t game_id;
   char     game_name[256];
   char     generic_post[2048];
   bool     cheats_on;
   bool     states_on;
   char     id[CL_SESSION_ID_LENGTH];
   time_t   last_status_update;
   bool     ready;

   cl_system_t system;
} cl_session_t;

#endif
