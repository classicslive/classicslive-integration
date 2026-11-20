#include "cl_action.h"
#include "cl_common.h"
#include "cl_dma.h"
#include "cl_json.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cl_error cl_act_no_process(cl_action_t *action)
{
  CL_UNUSED(action);
  return CL_OK;
}

cl_error cl_free_action(cl_action_t *action)
{
  action->argument_count = 0;
  cl_dma_free(action->arguments);
  action->arguments = NULL;
  action->type = CL_ACTTYPE_NO_PROCESS;
  action->function = cl_act_no_process;

  action->prev_action = NULL;
  action->next_action = NULL;

  return CL_ERR_CLIENT_RUNTIME;
}

static cl_error cl_print_counter_values(char *buffer, unsigned len)
{
  const cl_counter_t *counter;
  char counter_buffer[32];
  unsigned current_len, append_len;
  unsigned i;

  if (!buffer || len == 0)
    return CL_ERR_CLIENT_RUNTIME;

  for (i = 0; i < CL_COUNTERS_SIZE; i++)
  {
    counter = &script.current_page->counters[i];

    /* Get counter value to temporary buffer */
    /* Integers are stored sign-extended, so print them signed */
    if (cl_ctr_is_float(counter))
      snprintf(counter_buffer, sizeof(counter_buffer), "&c%u=%f",
               i, counter->floatval.fp);
    else
      snprintf(counter_buffer, sizeof(counter_buffer), "&c%u=" CL_FS64,
               i, counter->intval.i64);

    /* Check destination buffer can hold it */
    current_len = strlen(buffer);
    append_len = strlen(counter_buffer);
    if (current_len + append_len + 1 >= len)
      return CL_ERR_CLIENT_RUNTIME;

    /* Append to destination buffer */
    strncat(buffer, counter_buffer, len - current_len - 1);
  }

  return CL_OK;
}

/**
 * @brief Returns a copy of a counter's data. Used in cases where the requested
 *   counter should not be directly mutated.
 * @param source The source type of the requested counter.
 * @param offset An address or index relevant to the source type. For example,
 *   a memory note or counter index, or a memory address.
 * @return A mutable counter, or return CL_CTR_INVALID if unavailable.
 **/
static cl_counter_t cl_get_compare_value(cl_src_t source, cl_int64 offset)
{
  cl_counter_t counter;

  switch (source)
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
    /* Constant pointers are stored at canonical addresses, like memory notes */
    offset = (cl_int64)cl_memory_runtime_address((cl_addr_t)offset);
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

    if (!memnote)
      counter.type = CL_MEMTYPE_NOT_SET;
    else if (source == CL_SRCTYPE_CURRENT_RAM)
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
static cl_counter_t *cl_get_mutable_value(cl_src_t source, cl_int64 offset)
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

static cl_achievement_t *cl_find_achievement(unsigned id)
{
  unsigned i;

  for (i = 0; i < session.achievement_count; i++)
    if (session.achievements[i].id == id)
      return &session.achievements[i];

  return NULL;
}

#if !CL_HAVE_EDITOR

/**
 * Applies the "achievements" list the server returns from the achievement
 * and progress endpoints: the achievement itself plus any progress
 * achievements it advanced or completed. Announces new unlocks and progress,
 * except for subachievements, which only count toward their parents.
 */
static void cl_apply_achievement_updates(const char *json)
{
  cl_achievement_t *updates = NULL;
  unsigned count = 0;
  unsigned i;

  if (!json || cl_json_get_array((void**)&updates, &count, json,
      CL_JSON_KEY_ACHIEVEMENTS, CL_JSON_TYPE_ACHIEVEMENT) != CL_OK)
    return;

  for (i = 0; i < count; i++)
  {
    const cl_achievement_t *update = &updates[i];
    cl_achievement_t *ach = cl_find_achievement(update->id);
    cl_bool newly_unlocked, progressed, is_sub;

    if (!ach)
      continue;

    newly_unlocked = update->unlocked && !ach->unlocked;
    progressed = update->progress_total &&
                 update->progress_count != ach->progress_count;
    is_sub = (update->flags & CL_ACHIEVEMENT_FLAG_SUBACHIEVEMENT) != 0;

    ach->unlocked = ach->unlocked || update->unlocked;
    ach->flags = update->flags;
    if (update->progress_total)
    {
      ach->progress_count = update->progress_count;
      ach->progress_total = update->progress_total;
    }

    if (is_sub)
      continue;
    else if (newly_unlocked)
      cl_message(CL_MSG_INFO, "Achievement unlocked: %s", ach->title);
    else if (progressed && !ach->unlocked)
      cl_message(CL_MSG_INFO, "%s: %u / %u", ach->title,
                 ach->progress_count, ach->progress_total);
  }

  free(updates);
}

static CL_NETWORK_CB(cl_achievement_cb)
{
  CL_UNUSED(userdata);
  if (!response.error_code)
    cl_apply_achievement_updates(response.data);
}
#endif

/**
 * Submits the achievement whose ID the action's source gives. The ID can come
 * from a counter or memory note, so one action can unlock a different
 * achievement each time it runs, ie. "unlock achievement[current level]".
 * IDs this game doesn't have, and ones already unlocked or submitted since
 * the script was loaded, are ignored so nothing is posted every frame.
 * @param action The action, with a source type and value as arguments.
 * @return CL_OK.
 **/
static cl_error cl_act_post_achievement(cl_action_t *action)
{
  cl_counter_t ach_id = cl_get_compare_value(action->arguments[0].uintval,
                                             action->arguments[1].uintval);
  cl_achievement_t *ach;
#if !CL_HAVE_EDITOR
  char data[CL_POST_DATA_SIZE];
#endif

  if (ach_id.type == CL_MEMTYPE_NOT_SET)
    return CL_OK;
  ach = cl_find_achievement((unsigned)ach_id.intval.raw);
  if (!ach || ach->unlocked || ach->submitted)
    return CL_OK;

  /* Mark before notifying, in case showing the message re-enters the script */
  ach->submitted = CL_TRUE;

#if CL_HAVE_EDITOR
  cl_message(CL_MSG_INFO, "Editor mode: Achievement %u unlocked.", ach->id);
#else
  snprintf(data, CL_POST_DATA_SIZE, "achievement_id=%u", ach->id);
  cl_network_post_clint(CL_END_CLINT_ACHIEVEMENT, data, cl_achievement_cb, NULL);
#endif

  return CL_OK;
}

/**
 * Asks the server how far along a progress achievement is and announces it.
 * Progress itself comes from unlocking the achievement's subachievements.
 */
static cl_error cl_act_post_progress(cl_action_t *action)
{
  cl_counter_t ach_id = cl_get_compare_value(action->arguments[0].uintval,
                                             action->arguments[1].uintval);
#if !CL_HAVE_EDITOR
  char data[CL_POST_DATA_SIZE];

  snprintf(data, CL_POST_DATA_SIZE, "achievement_id=" CL_FU64, ach_id.intval.raw);
  cl_network_post_clint(CL_END_CLINT_PROGRESS, data, cl_achievement_cb, NULL);

  /* Clear this action so we don't spam progress requests every frame */
  cl_free_action(action);
#else
  cl_message(CL_MSG_INFO, "Editor mode: Progress requested for " CL_FU64 ".",
    ach_id.intval.raw);
#endif

  return CL_OK;
}

static cl_leaderboard_t *cl_find_leaderboard(unsigned id)
{
  unsigned i;

  for (i = 0; i < session.leaderboard_count; i++)
    if (session.leaderboards[i].id == id)
      return &session.leaderboards[i];

  return NULL;
}

#if !CL_HAVE_EDITOR
static CL_NETWORK_CB(cl_leaderboard_cb)
{
  /* Looked up again, in case the session ended while the request was out */
  const cl_leaderboard_t *ldb = cl_find_leaderboard((unsigned)(uintptr_t)userdata);
  const char *title = ldb && ldb->title[0] ? ldb->title : "Leaderboard";
  cl_bool success = CL_FALSE;
  cl_bool personal_best = CL_FALSE;
  char display[64];
  char reason[256];

  if (!response.data || cl_json_get(&success, response.data, CL_JSON_KEY_SUCCESS,
      CL_JSON_TYPE_BOOLEAN, sizeof(success)) != CL_OK || !success)
  {
    if (!response.data || cl_json_get(reason, response.data, CL_JSON_KEY_REASON,
        CL_JSON_TYPE_STRING, sizeof(reason)) != CL_OK)
      snprintf(reason, sizeof(reason), "No response from the server.");
    cl_message(CL_MSG_ERROR, "%s: score not submitted. %s", title, reason);
    return;
  }

  if (cl_json_get(display, response.data, CL_JSON_KEY_DISPLAY,
      CL_JSON_TYPE_STRING, sizeof(display)) != CL_OK)
    snprintf(display, sizeof(display), "submitted");
  cl_json_get(&personal_best, response.data, CL_JSON_KEY_PERSONAL_BEST,
    CL_JSON_TYPE_BOOLEAN, sizeof(personal_best));

  cl_message(CL_MSG_INFO, "%s: %s%s", title, display,
    personal_best ? " (new personal best!)" : "");
}
#endif

/**
 * Submits a score to the leaderboard whose ID the action's source gives,
 * along with every counter on the current page as c0..c15. The server works
 * out the score from those (and the rich memory note values, which every
 * request carries as m<id>) using the leaderboard's formula.
 */
static cl_error cl_act_post_leaderboard(cl_action_t *action)
{
  cl_counter_t ldb_id = cl_get_compare_value(action->arguments[0].uintval,
                                             action->arguments[1].uintval);
  cl_leaderboard_t *ldb;
  cl_bool ran_last_frame;
  char data[CL_POST_DATA_SIZE];

  if (ldb_id.type == CL_MEMTYPE_NOT_SET)
    return CL_OK;
  ldb = cl_find_leaderboard((unsigned)ldb_id.intval.raw);
  if (!ldb)
    return CL_OK;

  ran_last_frame = ldb->last_frame && ldb->last_frame + 1 == script.frame;
  ldb->last_frame = script.frame;
  if (ran_last_frame)
    return CL_OK;

  snprintf(data, CL_POST_DATA_SIZE, "leaderboard_id=%u", ldb->id);
  if (cl_print_counter_values(data, sizeof(data)) != CL_OK)
  {
    cl_message(CL_MSG_ERROR, "Unable to build leaderboard data.");
    return CL_OK;
  }

#if CL_HAVE_EDITOR
  cl_message(CL_MSG_INFO, "Editor mode: Leaderboard %u submitted.\n%s",
    ldb->id, data);
#else
  cl_network_post_clint(CL_END_CLINT_LEADERBOARD, data, cl_leaderboard_cb,
    (void*)(uintptr_t)ldb->id);
#endif

  return CL_OK;
}

static cl_error cl_act_compare(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(action->arguments[0].uintval, action->arguments[1].uintval);
  cl_counter_t right = cl_get_compare_value(action->arguments[2].uintval, action->arguments[3].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return CL_FALSE;

  switch (action->arguments[4].intval)
  {
  case CL_COMPARE_EQUAL:
    return cl_ctr_equal(&left, &right);
  case CL_COMPARE_GREATER:
    return cl_ctr_greater(&left, &right);
  case CL_COMPARE_LESS:
    return cl_ctr_lesser(&left, &right);
  case CL_COMPARE_NOT_EQUAL:
    return cl_ctr_not_equal(&left, &right);
  default:
    cl_free_action(action);
    return CL_FALSE;
  }
}

static cl_error cl_act_changed(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(CL_SRCTYPE_CURRENT_RAM, action->arguments[0].uintval);
  cl_counter_t right = cl_get_compare_value(CL_SRCTYPE_PREVIOUS_RAM, action->arguments[0].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return CL_FALSE;
  return cl_ctr_not_equal(&left, &right);
}

static cl_error cl_act_bits(cl_action_t *action)
{
  cl_counter_t left = cl_get_compare_value(action->arguments[0].uintval, action->arguments[1].uintval);
  cl_counter_t right = cl_get_compare_value(action->arguments[2].uintval, action->arguments[3].uintval);

  if (left.type == CL_MEMTYPE_NOT_SET || right.type == CL_MEMTYPE_NOT_SET)
    return CL_FALSE;

  /* TODO: Not exactly what we want */
  return (left.intval.raw & right.intval.raw) == right.intval.raw ? CL_TRUE : CL_FALSE;
}

static cl_error cl_act_write(cl_action_t *action)
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
        return CL_ERR_PARAMETER_NULL;
      else
        *ctr = right;

      return CL_OK;
    }
    default:
      cl_script_break(CL_TRUE, "Invalid srctype to write: %u", action->arguments[0].uintval);
      return CL_ERR_PARAMETER_INVALID;
    }
  }
}

/**
 * A template for command actions that only use one argument, for a counter
 * index that operates on itself.
 */
#define CL_TEMPLATE_CTR_UNARY \
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, \
                                           action->arguments[0].uintval); \
  if (!ctr || \
      ctr->type == CL_MEMTYPE_NOT_SET) \
    return CL_ERR_PARAMETER_INVALID; \
  else

/**
 * A template for command actions that use one argument for a mutable counter
 * index and two for a compare value lookup.
 */
#define CL_TEMPLATE_CTR_BINARY \
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, \
                                           action->arguments[0].uintval); \
  cl_counter_t src = cl_get_compare_value(action->arguments[1].uintval, \
                                          action->arguments[2].uintval); \
  if (!ctr || \
      ctr->type == CL_MEMTYPE_NOT_SET || \
      src.type == CL_MEMTYPE_NOT_SET) \
    return CL_ERR_PARAMETER_INVALID; \
  else

static cl_error cl_act_bitwise_and(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_and(ctr, &src);
  }
}

static cl_error cl_act_bitwise_complement(cl_action_t *action)
{
  CL_TEMPLATE_CTR_UNARY
  {
    return cl_ctr_complement(ctr);
  }
}

static cl_error cl_act_bitwise_or(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_or(ctr, &src);
  }
}

static cl_error cl_act_bitwise_xor(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_xor(ctr, &src);
  }
}

static cl_error cl_act_set(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    *ctr = src;
    return CL_OK;
  }
}

static cl_error cl_act_shift_left(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_left(ctr, &src);
  }
}

static cl_error cl_act_shift_right(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_shift_right(ctr, &src);
  }
}

static cl_error cl_act_multiplication(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_multiply(ctr, &src);
  }
}

static cl_error cl_act_division(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_divide(ctr, &src);
  }
}

static cl_error cl_act_addition(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_add(ctr, &src);
  }
}

static cl_error cl_act_modulo(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_modulo(ctr, &src);
  }
}

static cl_error cl_act_subtraction(cl_action_t *action)
{
  CL_TEMPLATE_CTR_BINARY
  {
    return cl_ctr_subtract(ctr, &src);
  }
}

static cl_error cl_act_change_ctr_type(cl_action_t *action)
{
  cl_counter_t *ctr = cl_get_mutable_value(CL_SRCTYPE_COUNTER, action->arguments[0].uintval);
  return ctr ? cl_ctr_change_type(ctr, action->arguments[1].uintval) : CL_ERR_PARAMETER_NULL;
}

static const cl_acttype_t action_types[] =
{
  { CL_ACTTYPE_NO_PROCESS, CL_FALSE, 0, 0, 0, cl_act_no_process },
  { CL_ACTTYPE_COMPARE,    CL_TRUE,  5, 5, 0, cl_act_compare },
  { CL_ACTTYPE_CHANGED,    CL_TRUE,  1, 1, 0, cl_act_changed },
  { CL_ACTTYPE_BITS,       CL_TRUE,  4, 4, 0, cl_act_bits },

  /* Counter arithmetic */
  { CL_ACTTYPE_ADDITION,       CL_FALSE, 3, 3, 0, cl_act_addition },
  { CL_ACTTYPE_SUBTRACTION,    CL_FALSE, 3, 3, 0, cl_act_subtraction },
  { CL_ACTTYPE_MULTIPLICATION, CL_FALSE, 3, 3, 0, cl_act_multiplication },
  { CL_ACTTYPE_DIVISION,       CL_FALSE, 3, 3, 0, cl_act_division },
  { CL_ACTTYPE_MODULO,         CL_FALSE, 3, 3, 0, cl_act_modulo },
  { CL_ACTTYPE_SET,            CL_FALSE, 3, 3, 0, cl_act_set },

  /* Counter bitwise arithmetic */
  { CL_ACTTYPE_AND,         CL_FALSE, 3, 3, 0, cl_act_bitwise_and },
  { CL_ACTTYPE_OR,          CL_FALSE, 3, 3, 0, cl_act_bitwise_or },
  { CL_ACTTYPE_XOR,         CL_FALSE, 3, 3, 0, cl_act_bitwise_xor },
  { CL_ACTTYPE_COMPLEMENT,  CL_FALSE, 1, 1, 0, cl_act_bitwise_complement },
  { CL_ACTTYPE_SHIFT_LEFT,  CL_FALSE, 3, 3, 0, cl_act_shift_left },
  { CL_ACTTYPE_SHIFT_RIGHT, CL_FALSE, 3, 3, 0, cl_act_shift_right },

  /* Direct value manipulation */
  { CL_ACTTYPE_WRITE,           CL_FALSE, 4, 4, 0, cl_act_write },
  { CL_ACTTYPE_CHANGE_CTR_TYPE, CL_FALSE, 2, 2, 0, cl_act_change_ctr_type },

  /* Website API calls */
  { CL_ACTTYPE_POST_ACHIEVEMENT, CL_FALSE, 2, 2,  0, cl_act_post_achievement },
  { CL_ACTTYPE_POST_LEADERBOARD, CL_FALSE, 2, 16, 2, cl_act_post_leaderboard },
  { CL_ACTTYPE_POST_PROGRESS,    CL_FALSE, 2, 16, 2, cl_act_post_progress },

  /* Control flow; handled by the script interpreter, never run directly */
  { CL_ACTTYPE_ELSE,        CL_FALSE, 0, 0, 0, cl_act_no_process },
  { CL_ACTTYPE_LOGICAL_OR,  CL_TRUE,  0, 0, 0, cl_act_no_process },

  { 0, CL_FALSE, 0, 0, 0, NULL }
};

cl_error cl_init_action(cl_action_t *action)
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

        return CL_OK;
      }
      action->function = cl_act_no_process;

      return CL_ERR_PARAMETER_INVALID;
    }
    else
      acttype++;
  }

  return CL_ERR_PARAMETER_INVALID;
}

cl_error cl_process_action(cl_action_t *action)
{
  cl_error result;

  if (!action)
    cl_script_break(CL_TRUE, "Attempted to process a NULL action.");
  else if (!action->function)
    cl_script_break(CL_TRUE, "Attempted to process an action with NULL "
      "implementation (action type %04X).", action->type);
  else if (action->breakpoint)
    cl_script_break(CL_FALSE, "User-defined breakpoint.");
  else
  {
    result = action->function(action);
    if (result == CL_OK)
    {
      action->executions++;
      return CL_OK;
    }
    return result;
  }

  return CL_ERR_CLIENT_RUNTIME;
}
