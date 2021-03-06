#ifndef CL_ACTION_H
#define CL_ACTION_H

#include "cl_common.h"

#define CL_ACTTYPE_NO_PROCESS         '0'
#define CL_ACTTYPE_BITS               'b'
#define CL_ACTTYPE_COMPARE            'c'
#define CL_ACTTYPE_HIT_COMPARE        'h'
#define CL_ACTTYPE_CHANGED            'n'
#define CL_ACTTYPE_POST_ACHIEVEMENT   'A'
#define CL_ACTTYPE_ADDITION           'D'
#define CL_ACTTYPE_POST_LEADERBOARD   'L'
#define CL_ACTTYPE_MULTIPLY           'M'
#define CL_ACTTYPE_POST_PROGRESS      'P'
#define CL_ACTTYPE_RESET_HITS         'R'
#define CL_ACTTYPE_SUBTRACTION        'S'
#define CL_ACTTYPE_WRITE              'W'

#define CL_CMPTYPE_IFEQUAL            1
#define CL_CMPTYPE_IFGREATER          2
#define CL_CMPTYPE_IFLESS             3
#define CL_CMPTYPE_IFNEQUAL           4

typedef struct cl_action_t
{
   uint32_t *arguments;
   uint8_t   argument_count;
   uint32_t  executions;
   bool    (*function)();
   uint8_t   indentation;
   char      type;

   /* TODO: Double-link actions together so the editor can easily insert new lines */ 
   struct cl_action_t *prev_action;
   struct cl_action_t *next_action;
} cl_action_t;

bool cl_free_action    (cl_action_t *action);

/* Assign the correct function pointer for the type of action */
bool cl_init_action    (cl_action_t *action);

/* Run the function and return whether it succeeded. We assume the pointer is not NULL. */
/* TODO: Do we need any safety checks here? */
bool cl_process_action (cl_action_t *action);

/* Private (documentation for processing a cl_action_t)

   'c' - Compare
   The most common action, here we return true if the comparison type
   evaluates to true between two defined cl_memnote_ts

   Args  - 5
   Arg 0 - Left source
   Arg 1 - Left offset
   Arg 2 - Right source
   Arg 3 - Right offset/value
   Arg 4 - Comparison type

bool cl_act_compare            (cl_action_t *action);

   'h' - Compare with RetroAchievements-style hits
   A hit is applied on every frame where the comparison is true, this then
   returns true if the hit counter meets its requirement.

   Args  - 7
   Arg 0 - Left source
   Arg 1 - Left offset
   Arg 2 - Right source
   Arg 3 - Right offset/value
   Arg 4 - Comparison type
   Arg 5 - Required hits to return true
   Arg 6 - Current number of hits

bool cl_act_hit_compare        (cl_action_t *action);

   'A' - Submit achievement unlock to the server 

   Args  - 1
   Arg 0 - Achievement ID 

bool cl_act_submit_achievement (cl_action_t *action);

   'N' - Tell the site we have begun a netplay session 

bool cl_act_netplay_start      (cl_action_t *action);

   'R' - Reset hits from RetroAchievements compatibility 
   Sets all hit counters ('h') in a given page to zero. 

   Args  - 1
   Arg 0 - Page index 

bool cl_act_reset_hits         (cl_action_t *action);

   'W' - Write a value into memory

   Args  - Multiple of 4
   Arg 0 - Destination type
   Arg 1 - Destination address
   Arg 2 - Source type
   Arg 3 - Source address/value

bool cl_act_write              (cl_action_t *action);
*/

#endif