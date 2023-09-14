#include <math.h>
#include <string.h>

#include "cl_action.h"
#include "cl_common.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"

bool cl_free_action(cl_action_t *action)
{
  action->argument_count = 0;
  free(action->arguments);
  action->arguments = NULL;
  action->type = CL_ACTTYPE_NO_PROCESS;

  action->prev_action = NULL;
  action->next_action = NULL;

  return false;
}

/**
 * @brief Returns a copy of a counter's data. Used in cases where the requested
 *   counter should not be directly mutated.
 * @param source The source type of the requested counter.
 * @param offset An address or index relevant to the source type. For example,
 *   a memory note or counter index, or a memory address.
 * @return A mutable counter, or return CL_CTR_INVALID if unavailable.
 **/
static cl_counter_t cl_get_compare_value(cl_src_t source, int64_t offset)
{
  cl_counter_t counter;

  switch (source)
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
    counter.type = CL_MEMTYPE_INT64;
    cl_ctr_store(&counter, &offset, CL_MEMTYPE_INT64);
    break;
  case CL_SRCTYPE_IMMEDIATE_FLOAT:
    counter.type = CL_MEMTYPE_DOUBLE;
    cl_ctr_store(&counter, &offset, CL_MEMTYPE_DOUBLE);
    break;
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
  {
    cl_memnote_t *memnote = cl_find_memnote((unsigned)offset);

    if (source == CL_SRCTYPE_CURRENT_RAM)
      counter = memnote->current;
    else if (source == CL_SRCTYPE_PREVIOUS_RAM)
      counter = memnote->previous;
    else /* if (source == CL_SRCTYPE_LAST_UNIQUE_RAM) warning silence */
      counter = memnote->last_unique;

    break;
  }
  case CL_SRCTYPE_COUNTER:
    if (offset < CL_COUNTERS_SIZE && offset >= 0)
      counter = script.current_page->counters[offset];
    else
      counter.type = CL_MEMTYPE_NOT_SET;
    break;
  case CL_SRCTYPE_ROM: /* TODO */
  default:
    counter.type = CL_MEMTYPE_NOT_SET;
  }

  return counter;
}

/**
 * Returns a pointer to a counter that can be mutated. Currently, this only
 *   includes the counters stored within script pages.
 * @param source The source type of the requested counter.
 * @param offset An address or index relevant to the source type.
 * @return A mutable counter, or NULL if unavailable.
 **/
static cl_counter_t *cl_get_mutable_value(cl_src_t source, int64_t offset)
{
  switch (source)
  {
  case CL_SRCTYPE_COUNTER:
    if (offset < CL_COUNTERS_SIZE && offset >= 0)
      return &script.current_page->counters[offset];
    else
      return NULL;
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_IMMEDIATE_FLOAT:
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
  case CL_SRCTYPE_ROM:
  default:
    return NULL;
  }

  return NULL;
}

static bool cl_act_post_achievement(cl_action_t *action)
{
  cl_counter_t ach_id = cl_get_compare_value(action->arguments[0].uintval,
                                             action->arguments[1].uintval);
  char data[CL_POST_DATA_SIZE];

  snprintf(data, CL_POST_DATA_SIZE, "ach_id=%u", ach_id.intval.i64);
  cl_network_post(CL_REQUEST_POST_ACHIEVEMENT, data, NULL);

  /* Clear this action so we don't re-submit the achievement */
  cl_free_action(action);

  return true;
}

static bool cl_act_post_progress(cl_action_t *action)
{
  unsigned key = action->arguments[0].uintval;
  char data[CL_POST_DATA_SIZE];

  snprintf(data, CL_POST_DATA_SIZE, "ach_id=%u", key);
  cl_network_post(CL_REQUEST_POST_PROGRESS, data, NULL);

  return true;
}

/** @todo Support optional values */
static bool cl_act_post_leaderboard(cl_action_t *action)
{
  cl_counter_t ldb_id = cl_get_compare_value(action->arguments[0].uintval,
                                             action->arguments[1].uintval);
  char data[CL_POST_DATA_SIZE];

  snprintf(data, CL_POST_DATA_SIZE, "ldb_id=%llu", ldb_id.intval.i64);
  cl_network_post(CL_REQUEST_POST_LEADERBOARD, data, NULL);
   
  return true;
}

static bool cl_act_compare(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(action->arguments[0].uintval, action->arguments[1].uintval);
  cl_counter_t right = cl_get_compare_value(action->arguments[2].uintval, action->arguments[3].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return cl_free_action(action);

  switch (action->arguments[4].intval)
  {
    case CL_CMPTYPE_IFEQUAL:
      return cl_ctr_equal(&left, &right);
    case CL_CMPTYPE_IFGREATER:
      return cl_ctr_greater(&left, &right);
    case CL_CMPTYPE_IFLESS:
      return cl_ctr_lesser(&left, &right);
    case CL_CMPTYPE_IFNEQUAL:
      return cl_ctr_not_equal(&left, &right);
    default:
      return cl_free_action(action);
  }
}

static bool cl_act_no_process(cl_action_t *action)
{
  CL_UNUSED(action);
  return false;
}

static bool cl_act_changed(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(CL_SRCTYPE_CURRENT_RAM, action->arguments[0].uintval);
  cl_counter_t right = cl_get_compare_value(CL_SRCTYPE_PREVIOUS_RAM, action->arguments[0].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return cl_free_action(action);
  else
    return cl_ctr_not_equal(&left, &right);
}

static bool cl_act_bits(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(action->arguments[0].uintval, action->arguments[1].uintval);
  cl_counter_t right = cl_get_compare_value(action->arguments[2].uintval, action->arguments[3].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return cl_free_action(action);

  /* TODO: Not exactly what we want */
  return (left.intval.raw & right.intval.raw) == right.intval.raw;
}

static bool cl_act_write(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(action->arguments[0].uintval, action->arguments[1].uintval);
  cl_counter_t right = cl_get_compare_value(action->arguments[2].uintval, action->arguments[3].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return cl_free_action(action);
  else
  {
    switch (action->arguments[0].uintval)
    {
    case CL_SRCTYPE_CURRENT_RAM:
      return cl_write_memnote_from_key(action->arguments[1].uintval, &right);
    case CL_SRCTYPE_COUNTER:
    {
      cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, action->arguments[1].uintval);
      if (!ctr)
        return false;
      else
        *ctr = right;

      return true;
    }
    default:
      cl_script_break(true, "Invalid srctype to write: %u", action->arguments[0].uintval);
      return false;
    }
  }
}

/**
 * A template for command actions that only use one argument, for a counter
 * index that operates on itself.
 **/
#define CL_TEMPLATE_CTR_UNARY \
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, \
                                           action->arguments[0].uintval); \
  if (!ctr || \
      ctr->type == CL_MEMTYPE_NOT_SET) \
    return false; \
  else

/**
 * A template for command actions that use one argument for a mutable counter
 * index and two for a compare value lookup.
 **/
#define CL_TEMPLATE_CTR_BINARY \
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, \
                                           action->arguments[0].uintval); \
  cl_counter_t src = cl_get_compare_value(action->arguments[1].uintval, \
                                          action->arguments[2].uintval); \
  if (!ctr || \
      ctr->type == CL_MEMTYPE_NOT_SET || \
      src.type == CL_MEMTYPE_NOT_SET) \
    return false; \
  else

static bool cl_act_bitwise_and(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_and(ctr, &src);
  }
}

static bool cl_act_bitwise_complement(cl_action_t *action)
{
  CL_TEMPLATE_CTR_UNARY
  {
    return cl_ctr_complement(ctr);
  }
}

static bool cl_act_bitwise_or(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_or(ctr, &src);
  }
}

static bool cl_act_bitwise_xor(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_xor(ctr, &src);
  }
}

static bool cl_act_shift_left(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_left(ctr, &src);
  }
}

static bool cl_act_shift_right(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_right(ctr, &src);
  }
}

static bool cl_act_multiplication(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_multiply(ctr, &src);
  }
}

static bool cl_act_division(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_divide(ctr, &src);
  }
}

static bool cl_act_addition(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_add(ctr, &src);
  }
}

static bool cl_act_modulo(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_modulo(ctr, &src);
  }
}

static bool cl_act_subtraction(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_subtract(ctr, &src);
  }
}

static bool cl_act_change_ctr_type(cl_action_t *action)
{
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, action->arguments[0].uintval);
  return ctr ? cl_ctr_change_type(ctr, action->arguments[1].uintval) : false;
}

static const cl_acttype_t action_types[] =
{
  { CL_ACTTYPE_NO_PROCESS, false, 0, 0, 0, cl_act_no_process },
  { CL_ACTTYPE_COMPARE,    true,  5, 5, 0, cl_act_compare },
  { CL_ACTTYPE_CHANGED,    true,  1, 1, 0, cl_act_changed },
  { CL_ACTTYPE_BITS,       true,  4, 4, 0, cl_act_bits },

  /* Counter arithmetic */
  { CL_ACTTYPE_ADDITION,       false, 3, 3, 0, cl_act_addition },
  { CL_ACTTYPE_SUBTRACTION,    false, 3, 3, 0, cl_act_subtraction },
  { CL_ACTTYPE_MULTIPLICATION, false, 3, 3, 0, cl_act_multiplication },
  { CL_ACTTYPE_DIVISION,       false, 3, 3, 0, cl_act_division },
  { CL_ACTTYPE_MODULO,         false, 3, 3, 0, cl_act_modulo },

  /* Counter bitwise arithmetic */
  { CL_ACTTYPE_AND,         false, 3, 3, 0, cl_act_bitwise_and },
  { CL_ACTTYPE_OR,          false, 3, 3, 0, cl_act_bitwise_or },
  { CL_ACTTYPE_XOR,         false, 3, 3, 0, cl_act_bitwise_xor },
  { CL_ACTTYPE_COMPLEMENT,  false, 1, 1, 0, cl_act_bitwise_complement },
  { CL_ACTTYPE_SHIFT_LEFT,  false, 3, 3, 0, cl_act_shift_left },
  { CL_ACTTYPE_SHIFT_RIGHT, false, 3, 3, 0, cl_act_shift_right },

  /* Direct value manipulation */
  { CL_ACTTYPE_WRITE,           false, 4, 4, 0, cl_act_write },
  { CL_ACTTYPE_CHANGE_CTR_TYPE, false, 2, 2, 0, cl_act_change_ctr_type },

  /* Website API calls */
  { CL_ACTTYPE_POST_ACHIEVEMENT, false, 2, 2,  0, cl_act_post_achievement },
  { CL_ACTTYPE_POST_LEADERBOARD, false, 2, 16, 2, cl_act_post_leaderboard },
  { CL_ACTTYPE_POST_PROGRESS,    false, 2, 16, 2, cl_act_post_progress },

  { 0, false, 0, 0, 0, NULL }
};

bool cl_init_action(cl_action_t *action)
{
  const cl_acttype_t *acttype = &action_types[0];

  while (acttype->function)
  {
    if (action->type == acttype->id)
    {
      if (action->argument_count > acttype->maximum_args)
        cl_message(CL_MSG_ERROR, "Action maximum_args error: %u > %u\n",
               action->argument_count,
               acttype->maximum_args);
      else if (action->argument_count < acttype->minimum_args)
        cl_message(CL_MSG_ERROR, "Action minimum_args error: %u < %u\n",
               action->argument_count,
               acttype->minimum_args);
      else if (((action->argument_count - acttype->minimum_args) &
               acttype->modulo_after_minimum) != 0)
        cl_message(CL_MSG_ERROR, "Action modulo_after_minimum error: %u % %u\n",
               action->argument_count - acttype->minimum_args,
               acttype->modulo_after_minimum);
      else
      {
        action->if_type = acttype->if_type;
        action->function = acttype->function;

        return true;
      }
      action->function = cl_act_no_process;

      return false;
    }
    else
      acttype++;
  }

  return false;
}

bool cl_process_action(cl_action_t *action)
{
  if (!action)
    cl_script_break(true, "Attempted to process a NULL action.");
  else if (!action->function)
    cl_script_break(true, "Attempted to process an action with NULL "
      "implementation (action type %04X).", action->type);
  else if (action->function(action))
  {
    action->executions++;
    return true;
  }

  return false;
}
