#include "cl_frontend.h"
#include "cl_memory.h"
#include "cl_script.h"

#include <stdarg.h>
#include <stdio.h>

cl_script_t script;

void cl_page_free(cl_page_t *page)
{
  unsigned i;

  for (i = 0; i < page->action_count; i++)
    cl_free_action(&page->actions[i]);
}

void cl_script_free(void)
{
  unsigned i;

  for (i = 0; i < script.page_count; i++)
    cl_page_free(&script.pages[i]);
}

bool cl_init_page(const char **pos, cl_page_t *page)
{
  cl_action_t *action      = NULL;
  cl_action_t *prev_action = NULL;
  unsigned     i, j;

  if (!cl_strto(pos, &page->action_count, sizeof(page->action_count), false))
    return false;
  page->actions = (cl_action_t*)calloc(page->action_count, sizeof(cl_action_t));

  cl_log("Actions: %u\n", page->action_count);

  for (i = 0; i < page->action_count; i++)
  {
    action = &page->actions[i];

    if (cl_strto(pos, &action->indentation, sizeof(action->indentation), false) &&
        cl_strto(pos, &action->type, sizeof(action->type), false) &&
        cl_strto(pos, &action->argument_count, sizeof(action->argument_count), false))
      cl_log("%u %u %u", page->actions[i].indentation, page->actions[i].type, page->actions[i].argument_count);
    
    if (!cl_init_action(action))
      return false;

    /* Allocate and initialize action arguments */
    action->arguments = (cl_arg_t*)calloc(action->argument_count, sizeof(cl_arg_t));
    for (j = 0; j < action->argument_count; j++)
    {
      if (!cl_strto(pos, &action->arguments[j], sizeof(cl_arg_t), true))
        return false;
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

  return page->actions != 0;
}

bool cl_script_init(const char **pos)
{
  script.status = CL_SCRSTATUS_INACTIVE;

  if (!cl_strto(pos, &script.page_count, sizeof(script.page_count), false))
    return false;
  else
  {
    unsigned i;

    script.pages = (cl_page_t*)calloc(script.page_count, sizeof(cl_page_t));
    for (i = 0; i < script.page_count; i++)
      if (!cl_init_page(pos, &script.pages[i]))
        return false;
    script.status = CL_SRCSTATUS_ACTIVE;

    return true;
  }
}

static unsigned cl_process_if_statements(cl_page_t *page, unsigned pos)
{
  unsigned current_indent = page->actions[pos].indentation;
  bool     evaluation     = true;
  unsigned i;

  for (i = pos; i < page->action_count; i++)
  {
    /* Make sure all if statements on the current indentation level are true */
    if (page->actions[i].if_type &&
        page->actions[i].indentation == current_indent)
    {
      if (evaluation)
        evaluation = cl_process_action(&page->actions[i]);
      else
        continue;
    }
    else if (current_indent < page->actions[i].indentation)
    {
      /* Indentation went one level deeper, should we follow it? */
      if (evaluation)
        return i;
      else
      {
        /* These conditions evaluated false, so skip all of their children */
        do
        {
          i++;
        } while (page->actions[i].indentation > current_indent &&
                 i < page->action_count);
        return i;
      }
    }
    else
      return i;
  }

  return page->action_count - 1;
}

bool cl_process_actions(cl_page_t *page)
{
  bool     success = true;
  unsigned i       = 0;

  while (i < page->action_count)
  {
    script.current_action = &page->actions[i];

    if (script.status != CL_SRCSTATUS_ACTIVE)
      break;
    else if (page->actions[i].if_type)
      i = cl_process_if_statements(page, i);
    else
    {
      /* TODO: Error handling if a direct command returns false? */
      success &= cl_process_action(&page->actions[i]);
      i++;
    }
  }

  return success;
}

bool cl_script_update(void)
{
  if (script.status != CL_SRCSTATUS_ACTIVE)
    return false;
  else
  {
    bool     success = true;
    unsigned i;

    for (i = 0; i < script.page_count; i++)
    {
      script.current_page = &script.pages[i];
      success &= cl_process_actions(script.current_page);
    }

    return success;
  }
}

void cl_script_break(bool fatal, const char *format, ...)
{
  va_list args;

  script.status = CL_SCRSTATUS_PAUSED;
  script.error_fatal = fatal;

  va_start(args, format);
  vsprintf(script.error_msg, format, args);
  va_end(args);

  /*
   * Show why the script errored out if the error is unrecoverable.
   * If in edit mode, have the content pause execution for debugging.
   */
  if (fatal)
  {
#if CL_HAVE_EDITOR
    cl_fe_pause();
#endif
    cl_message(CL_MSG_ERROR, script.error_msg);
  }
}
