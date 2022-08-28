#ifndef CL_ACTION_H
#define CL_ACTION_H

#include "cl_common.h"

enum
{
  CL_ACTTYPE_NO_PROCESS = 0,

  /* Counter arithmetic */
  CL_ACTTYPE_ADDITION,
  CL_ACTTYPE_SUBTRACTION,
  CL_ACTTYPE_MULTIPLICATION,
  CL_ACTTYPE_DIVISION,
  CL_ACTTYPE_MODULO,

  /* Counter bitwise arithmetic */
  CL_ACTTYPE_AND,
  CL_ACTTYPE_OR,
  CL_ACTTYPE_XOR,
  CL_ACTTYPE_COMPLEMENT,
  CL_ACTTYPE_SHIFT_LEFT,
  CL_ACTTYPE_SHIFT_RIGHT,

  CL_ACTTYPE_WRITE,
  CL_ACTTYPE_CHANGE_CTR_TYPE,

  CL_ACTTYPE_COMPARE,
  CL_ACTTYPE_CHANGED,
  CL_ACTTYPE_BITS,

  /* API calls */
  CL_ACTTYPE_POST_ACHIEVEMENT,
  CL_ACTTYPE_POST_LEADERBOARD,
  CL_ACTTYPE_POST_PROGRESS,
  CL_ACTTYPE_POST_POLL,
  CL_ACTTYPE_POST_INFO,
};

typedef struct
{
  unsigned id;
  bool     if_type;
  unsigned minimum_args;
  unsigned maximum_args;
  bool    (*function)();
} cl_acttype_t;

#define CL_CMPTYPE_IFEQUAL   1
#define CL_CMPTYPE_IFGREATER 2
#define CL_CMPTYPE_IFLESS    3
#define CL_CMPTYPE_IFNEQUAL  4

typedef struct cl_action_t
{
   cl_arg_t *arguments;
   unsigned  argument_count;
   unsigned  executions;
   bool    (*function)();
   bool      if_type;
   unsigned  indentation;
   unsigned  type;

   /* TODO: Double-link actions together so the editor can easily insert new lines */ 
   struct cl_action_t *prev_action;
   struct cl_action_t *next_action;
} cl_action_t;

bool cl_free_action(cl_action_t *action);

/* Assign the correct function pointer for the type of action */
bool cl_init_action(cl_action_t *action);

/* Run the function and return whether it succeeded. */
bool cl_process_action(cl_action_t *action);

#endif
