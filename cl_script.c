#include "cl_script.h"

#include "cl_abi.h"
#include "cl_dma.h"

#include <stdarg.h>
#include <stdio.h>

cl_script_t script;

static void cl_page_free(cl_page_t *page)
{
  unsigned i;

  if (!page)
    return;
  for (i = 0; i < page->action_count; i++)
    cl_free_action(&page->actions[i]);
  cl_dma_free(page->actions);
  page->actions = NULL;
  page->action_count = 0;
  cl_dma_free(page);
}

void cl_script_free(void)
{
  unsigned i;

  for (i = 0; i < script.page_count; i++)
    cl_page_free(&script.pages[i]);
  script.pages = NULL;
  script.page_count = 0;
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

    if (cl_strto(pos, &action->indentation, sizeof(action->indentation), CL_FALSE) == CL_OK &&
        cl_strto(pos, &action->type, sizeof(action->type), CL_FALSE) == CL_OK &&
        cl_strto(pos, &action->argument_count, sizeof(action->argument_count), CL_FALSE) == CL_OK)
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

  return CL_OK;
}

cl_error cl_script_init(const char **pos)
{
  cl_error error;
  unsigned i;

  script.status = CL_SCRIPT_STATUS_INACTIVE;

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

static unsigned cl_process_if_statements(cl_page_t *page, unsigned pos)
{
  unsigned current_indent = page->actions[pos].indentation;
  cl_bool evaluation = CL_TRUE;
  unsigned i = pos;

  if (!page || !page->actions ||
      page->action_count == 0 || pos >= page->action_count)
    return page->action_count;
  current_indent = page->actions[pos].indentation;
  while (i < page->action_count)
  {
    cl_action_t *action = &page->actions[i];

    if (action->if_type && action->indentation == current_indent)
    {
      if (evaluation)
        evaluation = cl_process_action(action);
      i++;
    }
    else if (action->indentation > current_indent)
    {
      if (evaluation)
        return i;
      else
      {
        do
        {
          i++;
        } while (i < page->action_count &&
                 page->actions[i].indentation > current_indent);
        return i;
      }
    }
    else
      return i;
  }

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
