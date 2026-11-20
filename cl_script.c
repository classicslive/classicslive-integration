#include "cl_script.h"

#include "cl_abi.h"
#include "cl_dma.h"
#include "cl_main.h"

#include <stdarg.h>
#include <stdio.h>

cl_script_t script;

/**
 * Frees a page's actions. The page itself is part of the script's page array,
 * which is freed with the script.
 * @param page The page to empty.
 **/
static void cl_page_free(cl_page_t *page)
{
  unsigned i;

  if (!page || !page->actions)
    return;
  for (i = 0; i < page->action_count; i++)
    cl_free_action(&page->actions[i]);
  cl_dma_free(page->actions);
  page->actions = NULL;
  page->action_count = 0;
}

void cl_script_free(void)
{
  unsigned i;

  if (script.pages)
  {
    for (i = 0; i < script.page_count; i++)
      cl_page_free(&script.pages[i]);
    cl_dma_free(script.pages);
  }
  script.pages = NULL;
  script.page_count = 0;
  script.current_page = NULL;
  script.current_action = NULL;
  script.status = CL_SCRIPT_STATUS_INACTIVE;
}

/**
 * @param page The page holding the actions.
 * @param index The index of the action to check; may be out of range.
 * @param indent The indentation the action must have.
 * @return Whether the action is a condition (not an OR) at the indentation.
 **/
static cl_bool cl_is_condition(const cl_page_t *page, unsigned index,
  unsigned indent)
{
  const cl_action_t *action;

  if (index >= page->action_count)
    return CL_FALSE;
  action = &page->actions[index];

  return action->if_type && action->type != CL_ACTTYPE_LOGICAL_OR &&
         action->indentation == indent ? CL_TRUE : CL_FALSE;
}

/**
 * Checks that every OR sits between two conditions at its indentation, and
 * that every ELSE follows a condition chain (and its body) at its indentation.
 * @param page The page to check.
 * @return CL_OK if the page is valid, or an error code describing why not.
 **/
static cl_error cl_validate_page(const cl_page_t *page)
{
  const cl_action_t *action;
  const char *problem = NULL;
  unsigned i = 0;
  unsigned j;

  while (i < page->action_count && !problem)
  {
    action = &page->actions[i];
    if (action->type == CL_ACTTYPE_LOGICAL_OR)
    {
      if (i == 0 || !cl_is_condition(page, i - 1, action->indentation) ||
          !cl_is_condition(page, i + 1, action->indentation))
        problem = "OR must sit between two conditions at the same indentation";
    }
    else if (action->type == CL_ACTTYPE_ELSE)
    {
      j = i;
      while (j > 0 && page->actions[j - 1].indentation > action->indentation)
        j--;
      if (j == 0 || !cl_is_condition(page, j - 1, action->indentation))
        problem = "ELSE must follow a condition at the same indentation";
    }
    if (!problem)
      i++;
  }
  if (problem)
  {
    /* Numbered from 1, matching the editor's "Error in action" messages */
    cl_message(CL_MSG_ERROR, "Invalid script action %u: %s.", i + 1, problem);
    return CL_ERR_PARAMETER_INVALID;
  }

  return CL_OK;
}

static cl_error cl_init_page(const char **pos, cl_page_t *page)
{
  cl_action_t *action      = NULL;
  cl_action_t *prev_action = NULL;
  unsigned     i, j;

  if (cl_strto(pos, &page->action_count, sizeof(page->action_count), CL_FALSE) != CL_OK)
    return CL_ERR_PARAMETER_INVALID;
  page->actions = (cl_action_t*)cl_dma_alloc(page->action_count * sizeof(cl_action_t), CL_TRUE);
  if (!page->actions)
    return CL_ERR_PARAMETER_NULL;

  cl_log("Actions: %u\n", page->action_count);

  for (i = 0; i < page->action_count; i++)
  {
    action = &page->actions[i];

    if (cl_strto(pos, &action->indentation, sizeof(action->indentation), CL_FALSE) != CL_OK ||
        cl_strto(pos, &action->type, sizeof(action->type), CL_FALSE) != CL_OK ||
        cl_strto(pos, &action->argument_count, sizeof(action->argument_count), CL_FALSE) != CL_OK)
      return CL_ERR_PARAMETER_INVALID;
    cl_log("%u %u %u", page->actions[i].indentation, page->actions[i].type, page->actions[i].argument_count);

    if (cl_init_action(action) != CL_OK)
      return CL_ERR_CLIENT_RUNTIME;

    /* Allocate and initialize action arguments */
    action->arguments = (cl_arg_t*)cl_dma_alloc(action->argument_count * sizeof(cl_arg_t), CL_TRUE);
    if (!action->arguments)
      return CL_ERR_PARAMETER_NULL;
    for (j = 0; j < action->argument_count; j++)
    {
      if (cl_strto(pos, &action->arguments[j], sizeof(cl_arg_t), CL_TRUE) != CL_OK)
        return CL_ERR_PARAMETER_INVALID;
      cl_log(" %lld", page->actions[i].arguments[j].uintval);
    }

    /* Double-linked list */
    action->prev_action = prev_action;
    action->next_action = NULL;
    if (prev_action)
      prev_action->next_action = action;

    cl_log("\n");
  }
  /* Zero-init the page's counters */
  for (i = 0; i < CL_COUNTERS_SIZE; i++)
  {
    page->counters[i].type = CL_MEMTYPE_INT64;
    cl_ctr_store_int(&page->counters[i], 0);
  }

  cl_log("End of page.\n");

  return cl_validate_page(page);
}

cl_error cl_script_init(const char **pos)
{
  cl_error error;
  unsigned i;

  script.status = CL_SCRIPT_STATUS_INACTIVE;

  /* A newly loaded script may submit every achievement again */
  for (i = 0; i < session.achievement_count; i++)
    session.achievements[i].submitted = CL_FALSE;

  if (cl_strto(pos, &script.page_count, sizeof(script.page_count), CL_FALSE) != CL_OK)
    return CL_ERR_PARAMETER_INVALID;

  script.pages = (cl_page_t*)cl_dma_alloc(script.page_count * sizeof(cl_page_t), CL_TRUE);
  if (!script.pages)
    return CL_ERR_PARAMETER_NULL;

  for (i = 0; i < script.page_count; i++)
  {
    error = cl_init_page(pos, &script.pages[i]);
    if (error != CL_OK)
      return error;
  }
  script.status = CL_SCRIPT_STATUS_ACTIVE;

  return CL_OK;
}

/**
 * Skips the block of actions indented under an action.
 * @param page The page holding the actions.
 * @param pos The index of the first action that may be in the block.
 * @param indent The indentation of the action that owns the block.
 * @return The index of the first action not indented deeper than indent.
 **/
static unsigned cl_skip_block(const cl_page_t *page, unsigned pos,
  unsigned indent)
{
  while (pos < page->action_count && page->actions[pos].indentation > indent)
    pos++;

  return pos;
}

/**
 * Evaluates the condition chain starting at an action, as an AND of clauses
 * where each clause is one condition or several joined by OR actions.
 * Conditions after the result is known are not evaluated.
 * @param page The page holding the actions.
 * @param pos The index of the first condition in the chain.
 * @return The index of the next action to process: the start of the chain's
 * body if it was true, else the start of its ELSE block, if any, or the
 * first action after the chain and its body.
 **/
static unsigned cl_process_if_statements(cl_page_t *page, unsigned pos)
{
  cl_action_t *action;
  unsigned indent, i;
  cl_bool chain = CL_TRUE;
  cl_bool clause = CL_TRUE;
  cl_bool joined = CL_FALSE;

  if (!page || !page->actions || pos >= page->action_count)
    return page ? page->action_count : 0;
  indent = page->actions[pos].indentation;

  for (i = pos; i < page->action_count; i++)
  {
    action = &page->actions[i];
    if (!action->if_type || action->indentation != indent)
      break;
    else if (action->type == CL_ACTTYPE_LOGICAL_OR)
      joined = CL_TRUE;
    else
    {
      /* A condition not joined by OR closes the previous clause */
      if (!joined)
      {
        chain = chain && clause;
        clause = CL_FALSE;
      }
      if (chain && !clause)
        clause = cl_process_action(action) == CL_TRUE ? CL_TRUE : CL_FALSE;
      joined = CL_FALSE;
    }
  }

  if (chain && clause)
    return i;

  /* Skip the body, and enter the else block if one follows */
  i = cl_skip_block(page, i, indent);
  if (i < page->action_count &&
      page->actions[i].type == CL_ACTTYPE_ELSE &&
      page->actions[i].indentation == indent)
    i++;

  return i;
}

static cl_error cl_process_actions(cl_page_t *page)
{
  cl_error error = CL_OK;
  unsigned i = 0;

  if (!page || !page->actions || page->action_count == 0)
    return CL_ERR_PARAMETER_NULL;

  while (i < page->action_count)
  {
    script.current_action = &page->actions[i];

    if (script.status != CL_SCRIPT_STATUS_ACTIVE)
      break;
    /* Reaching an else in order means its chain was true, so skip it */
    else if (page->actions[i].type == CL_ACTTYPE_ELSE)
      i = cl_skip_block(page, i + 1, page->actions[i].indentation);
    else if (page->actions[i].if_type)
      i = cl_process_if_statements(page, i);
    else
    {
      if (cl_process_action(&page->actions[i]) != CL_OK)
        error = CL_ERR_CLIENT_RUNTIME;
      i++;
    }
  }

  return error;
}

cl_error cl_script_update(void)
{
  cl_error error = CL_OK;
  cl_error page_error;
  unsigned i;

  if (script.status != CL_SCRIPT_STATUS_ACTIVE)
    return CL_ERR_CLIENT_RUNTIME;

  for (i = 0; i < script.page_count; i++)
  {
    script.current_page = &script.pages[i];
    page_error = cl_process_actions(script.current_page);
    if (page_error != CL_OK)
      error = page_error;
  }

  return error;
}

void cl_script_break(cl_bool fatal, const char *format, ...)
{
  va_list args;

  script.status = CL_SCRIPT_STATUS_PAUSED;
  script.error_fatal = fatal;

  va_start(args, format);
  vsnprintf(script.error_msg, sizeof(script.error_msg), format, args);
  va_end(args);

  /*
   * Show why the script errored out if the error is unrecoverable.
   * If in edit mode, have the content pause execution for debugging.
   */
  if (fatal)
  {
#if CL_HAVE_EDITOR
    cl_abi_set_pause(1);
#endif
    cl_message(CL_MSG_ERROR, script.error_msg);
  }
}
