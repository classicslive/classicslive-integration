#ifndef CL_ACTION_H
#define CL_ACTION_H

#include "cl_common.h"

#define CL_ACTTYPE_NO_PROCESS         '0'
#define CL_ACTTYPE_BITS               'b'
#define CL_ACTTYPE_COMPARE            'c'
#define CL_ACTTYPE_CHANGED            'n'
#define CL_ACTTYPE_POST_ACHIEVEMENT   'A'
#define CL_ACTTYPE_ADDITION           'D'
#define CL_ACTTYPE_POST_LEADERBOARD   'L'
#define CL_ACTTYPE_MULTIPLY           'M'
#define CL_ACTTYPE_AND                'N'
#define CL_ACTTYPE_OR                 'O'
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

/* Run the function and return whether it succeeded. */
bool cl_process_action (cl_action_t *action);

#endif