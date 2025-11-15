#ifndef CL_ACTION_H
#define CL_ACTION_H

#include "cl_types.h"

typedef enum
{
  CL_ACTTYPE_NO_PROCESS = 0,

  /* Counter arithmetic */
  CL_ACTTYPE_ADDITION,
  CL_ACTTYPE_SUBTRACTION,
  CL_ACTTYPE_MULTIPLICATION,
  CL_ACTTYPE_DIVISION,
  CL_ACTTYPE_SET,
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

  CL_ACTTYPE_ELSE,

  CL_ACTTYPE_SIZE
} cl_action_id;

typedef struct
{
  cl_action_id id;

  /* Whether or not to evaluate the action as a conditional */
  bool if_type;

  /* The minimum number of arguments allowed for this action type */
  unsigned minimum_args;

  /* The maximum number of arguments allowed for this action type */
  unsigned maximum_args;

  /* The number of arguments used to store each optional */
  unsigned modulo_after_minimum;

  /* The function of the action */
  bool (*function)();
} cl_acttype_t;

enum
{
  CL_CMPTYPE_INVALID = 0,

  CL_CMPTYPE_IFEQUAL,
  CL_CMPTYPE_IFGREATER,
  CL_CMPTYPE_IFLESS,
  CL_CMPTYPE_IFNEQUAL,
  
  CL_CMPTYPE_SIZE
};

typedef struct cl_action_t
{
  cl_arg_t *arguments;
  unsigned argument_count;
  unsigned executions;
  bool breakpoint;
  bool (*function)(struct cl_action_t *action);
  bool if_type;
  unsigned indentation;
  cl_action_id type;

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
