#include <math.h>
#include <string.h>

#include "cl_action.h"
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

cl_counter_t cl_get_compare_value(cl_src_t source, int64_t offset)
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
    cl_memnote_t *memnote = cl_find_memnote(*(uint32_t*)offset);

    if (source == CL_SRCTYPE_CURRENT_RAM)
      counter = memnote->current;
    else if (source == CL_SRCTYPE_PREVIOUS_RAM)
      counter = memnote->previous;
    else /* if (source == CL_SRCTYPE_LAST_UNIQUE_RAM) warning silence */
      counter = memnote->last_unique;

    break;
  }
  default:
    counter.type = CL_MEMTYPE_NOT_SET;
  }

  return counter;
}

static bool cl_act_post_achievement(cl_action_t *action)
{
   if (action->argument_count != 1)
      return cl_free_action(action);
   else
   {
      uint32_t key = action->arguments[0];
      char     data[CL_POST_DATA_SIZE];

      snprintf(data, CL_POST_DATA_SIZE, "ach_id=%u", key);
      cl_network_post(CL_REQUEST_POST_ACHIEVEMENT, data, NULL);

      /* Clear this action so we don't re-submit the achievement */
      cl_free_action(action);
   }

   return true;
}

static bool cl_act_post_achievement_progress(cl_action_t *action)
{
   if (action->argument_count != 1)
      return cl_free_action(action);
   else
   {
      uint32_t key = action->arguments[0];
      char     data[CL_POST_DATA_SIZE];

      snprintf(data, CL_POST_DATA_SIZE, "ach_id=%u", key);
      cl_network_post(CL_REQUEST_POST_PROGRESS, data, NULL);
   }
   
   return true;
}

static bool cl_act_post_leaderboard(cl_action_t *action)
{
   if (action->argument_count != 1)
      return cl_free_action(action);
   else
   {
      uint32_t key = action->arguments[0];
      char     data[CL_POST_DATA_SIZE];

      snprintf(data, CL_POST_DATA_SIZE, "ldb_id=%u", key);
      cl_network_post(CL_REQUEST_POST_LEADERBOARD, data, NULL);
   }
   
   return true;
}

static bool cl_act_compare(cl_action_t *action)
{
  if (action->argument_count < 5)
    return cl_free_action(action);
  else
  {
    cl_counter_t left = cl_get_compare_value(action->arguments[0], action->arguments[1]);
    cl_counter_t right = cl_get_compare_value(action->arguments[2], action->arguments[3]);

    if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
      return cl_free_action(action);

    switch (action->arguments[4])
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
}

static bool cl_act_no_process(cl_action_t *action)
{
  return false;
}

static bool cl_act_changed(cl_action_t *action)
{
  if (action->argument_count != 3)
    return cl_free_action(action);
  else
  {
    cl_counter_t left = cl_get_compare_value(action->arguments[0], action->arguments[1]);
    cl_counter_t right = cl_get_compare_value(action->arguments[2], action->arguments[3]);

    if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
      return cl_free_action(action);
    else
      return cl_ctr_not_equal(&left, &right);
  }
}

static bool cl_act_bits(cl_action_t *action)
{
  if (action->argument_count != 4)
    return cl_free_action(action);
  else
  {
    cl_counter_t left = cl_get_compare_value(action->arguments[0], action->arguments[1]);
    cl_counter_t right = cl_get_compare_value(action->arguments[2], action->arguments[3]);

    if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
      return cl_free_action(action);

    /* TODO: Not exactly what we want */
    return (left.intval & right.intval) == right.intval;
  }
}

static bool cl_act_write(cl_action_t *action)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
       cl_counter_t left = cl_get_compare_value(action->arguments[0], action->arguments[1]);
       cl_counter_t right = cl_get_compare_value(action->arguments[2], action->arguments[3]);

       if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
         return cl_free_action(action);
      else
      {
         switch (left.type)
         {
         case CL_SRCTYPE_CURRENT_RAM:
            return cl_write_memnote_from_key(action->arguments[1], &right);
         default:
            return false;
         }
      }
      
      return true;
   }
}

/*
   A template for command actions that only use one argument, for a counter 
   index that operates on itself.
*/
#define CL_TEMPLATE_CTR_UNARY \
  cl_counter_t ctr = cl_get_compare_value(CL_SRCTYPE_COUNTER, action->arguments[0]); \
  if (ctr.type == CL_MEMTYPE_NOT_SET) \
    return false; \
  else

/*
   A template for command actions that use one argument for a counter index and
   two for a compare value lookup.
*/
#define CL_TEMPLATE_CTR_BINARY \
  cl_counter_t ctr, src; \
  if (action->argument_count != 3) \
    return false; \
  ctr = cl_get_compare_value(CL_SRCTYPE_COUNTER, action->arguments[0]); \
  src = cl_get_compare_value(action->arguments[1], action->arguments[2]); \
  if (ctr.type == CL_MEMTYPE_NOT_SET || src.type == CL_MEMTYPE_NOT_SET) \
    return false; \
  else

static bool cl_act_bitwise_and(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_and(&ctr, &src.intval);
  }
}

static bool cl_act_bitwise_complement(cl_action_t *action)
{
  CL_TEMPLATE_CTR_UNARY
  {
    return cl_ctr_complement(&ctr);
  }
}

static bool cl_act_bitwise_or(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_or(&ctr, &src.intval);
  }
}

static bool cl_act_bitwise_xor(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_xor(&ctr, &src.intval);
  }
}

static bool cl_act_shift_left(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_left(&ctr, &src.intval);
  }
}

static bool cl_act_shift_right(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_right(&ctr, &src.intval);
  }
}

static bool cl_act_multiplication(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_multiply(&ctr, &src);
  }
}

static bool cl_act_division(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_divide(&ctr, &src);
  }
}

static bool cl_act_addition(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_add(&ctr, &src);
  }
}

static bool cl_act_modulo(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_modulo(&ctr, &src);
  }
}

static bool cl_act_subtraction(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_subtract(&ctr, &src);
  }
}

static const cl_acttype_t action_types[] =
{
  { CL_ACTTYPE_NO_PROCESS, false, 0, 0, cl_act_no_process },
/*{ CL_ACTTYPE_BITS,       true,  4, 4, cl_act_bits },*/
  { CL_ACTTYPE_COMPARE,    true,  5, 5, cl_act_compare },

  /* Counter math */
  { CL_ACTTYPE_ADDITION,       false, 3, 3, cl_act_addition },
  { CL_ACTTYPE_SUBTRACTION,    false, 3, 3, cl_act_subtraction },
  { CL_ACTTYPE_MULTIPLICATION, false, 3, 3, cl_act_multiplication },
/*{ CL_ACTTYPE_DIVISION,       false, 3, 3, cl_act_division },*/
  { CL_ACTTYPE_ADDITION,       false, 3, 3, cl_act_modulo },

  /* Website API calls */
  { CL_ACTTYPE_POST_ACHIEVEMENT, false, 1, 1, cl_act_post_achievement },
  { CL_ACTTYPE_POST_LEADERBOARD, false, 1, 15, cl_act_post_leaderboard },

  { 0, false, 0, 0, NULL }
};

bool cl_init_action(cl_action_t *action)
{
  const cl_acttype_t *acttype = &action_types[0];

  while (acttype->function)
  {
    if (action->type == acttype->id)
    {
      if (action->argument_count > acttype->maximum_args)
        action->function = cl_act_no_process;
      else if (action->argument_count < acttype->minimum_args)
        action->function = cl_act_no_process;
      else
      {
        action->if_type = acttype->if_type;
        action->function = acttype->function;

        return true;
      }
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
