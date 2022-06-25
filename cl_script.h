#ifndef CL_SCRIPT_H
#define CL_SCRIPT_H

enum
{
   CL_SCRSTATUS_INACTIVE = 0,

   CL_SRCSTATUS_ACTIVE,
   CL_SCRSTATUS_PAUSED,

   CL_SCRSTATUS_LAST
};

/* TODO: Arbitrary! Have pages allocate more/less depending on need */
#define CL_COUNTERS_SIZE 16

#include "cl_action.h"

typedef struct cl_counter_t
{
   unsigned type;
   uint32_t value;
} cl_counter_t;

typedef struct cl_page_t
{
   cl_action_t *actions;
   unsigned     action_count;

   /* Temporary values (bitflags, counters) we can use for logic */
   cl_counter_t counters[CL_COUNTERS_SIZE];

   uint32_t     flags;
} cl_page_t;

typedef struct cl_script_t
{
   cl_page_t *pages;
   unsigned   page_count;

   /* Which action in a script is currently being processed. */
   cl_action_t *current_action;

   /* Which page in a script is currently being processed. */
   cl_page_t *current_page;

   bool evaluation;

   /* The status of the script. For example, CL_SRCSTATUS_ACTIVE. */
   uint8_t status;

   /* Whether or not the last script break was triggered by a fatal error. */
   bool error_fatal;

   /* A message describing the cause of the last script break. */
   char error_msg[256];
} cl_script_t;

/**
 * Frees the current script and all associated values.
 */
void cl_script_free(void);

uint32_t* cl_get_counter(uint8_t counter_number);

/**
 * Copies the value of a counter into a given buffer.
 * @param buffer The destination buffer.
 * @param counter_num The index of the source counter.
 * @return Whether the copy succeeded.
 */
bool cl_get_counter_value(void *buffer, uint8_t counter_num);

bool cl_script_init(const char **pos);

bool cl_update_script(void);

/**
 * Signals to halt processing of the script and core. Used when debugging 
 * scripts.
 * @param fatal Whether or not the break reason included a fatal error.
 * @param format A format string with arguments, to specify a reason for breaking. 
 **/
void cl_script_break(bool fatal, const char *format, ...);

extern cl_script_t script;

#endif