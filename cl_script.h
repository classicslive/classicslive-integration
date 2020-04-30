#ifndef CL_SCRIPT_H
#define CL_SCRIPT_H

#define CL_PGFLAG_ACTIVE       0
#define CL_PGFLAG_CORE         1
#define CL_PGFLAG_GLOBAL_PAUSE 2

/* TODO: Arbitrary! Have pages allocate more/less depending on need */
#define CL_COUNTERS_SIZE       16

#include "cl_action.h"

typedef struct cl_page_t
{
   cl_action_t *actions;
   uint16_t     action_count;

   /* Temporary values (bitflags, counters) we can use for logic */
   uint32_t     counters[16];

   uint32_t     flags;
} cl_page_t;

typedef struct cl_script_t
{
   cl_page_t *pages;
   uint16_t   page_count;

   /* Used while processing */
   cl_page_t *current_page;
   bool       evaluation;
} cl_script_t;

/* Public */
void      cl_free_script       (cl_script_t *script);
/*
uint32_t* cl_get_counter       (uint8_t counter_number);
bool      cl_get_counter_value (uint32_t *buffer, uint8_t counter_num);
*/
bool      cl_init_script       (const char **pos, cl_script_t *script);
bool      cl_update_script     (cl_script_t *script);

/* Private
bool     cl_is_if_statement       (uint8_t type);
bool     cl_process_actions       (cl_page_t *page);
uint32_t cl_process_if_statements (cl_page_t *page, uint32_t pos);
*/

#endif