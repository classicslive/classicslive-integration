#ifndef CL_SCRIPT_C
#define CL_SCRIPT_C

#include <stdarg.h>
#include <string.h>

#include "cl_script.h"
#include "frontend/cl_frontend.h"

cl_script_t script;

void cl_free_page(cl_page_t *page)
{
   uint32_t i;

   for (i = 0; i < page->action_count; i++)
      cl_free_action(&page->actions[i]);
}

void cl_free_script()
{
   uint32_t i;

   for (i = 0; i < script.page_count; i++)
      cl_free_page(&script.pages[i]);
}

uint32_t* cl_get_counter(uint8_t counter_num)
{
   if (!script.current_page || CL_COUNTERS_SIZE <= counter_num)
      return NULL;
   else
      return &script.current_page->counters[counter_num].value;
}

bool cl_get_counter_value(uint32_t *buffer, uint8_t counter_num)
{
   if (!script.current_page || CL_COUNTERS_SIZE <= counter_num)
      return false;
   else
      *buffer = script.current_page->counters[counter_num].value;

   return true;
}

bool cl_init_page(const char **pos, cl_page_t *page)
{
   cl_action_t *action      = NULL;
   cl_action_t *prev_action = NULL;
   uint32_t     i;
   uint8_t      j;
   
   if (!cl_strto(pos, &page->action_count, sizeof(page->action_count), false))
      return false;
   page->actions = (cl_action_t*)calloc(page->action_count, sizeof(cl_action_t));

   cl_log("Actions: %u\n", page->action_count);
   
   for (i = 0; i < page->action_count; i++)
   {
      action = &page->actions[i];

      if (!cl_strto(pos, &action->indentation,    1, false) ||
          !cl_strto(pos, &action->type,           1, false) ||
          !cl_strto(pos, &action->argument_count, 1, false) ||
          !cl_init_action(action))
         return false;

      cl_log("%u %c %u", page->actions[i].indentation, page->actions[i].type, page->actions[i].argument_count);

      /* Allocate and initialize action arguments */
      action->arguments = (uint32_t*)calloc(action->argument_count, sizeof(uint32_t));
      for (j = 0; j < action->argument_count; j++)
      {
         if (!cl_strto(pos, &action->arguments[j], 4, false))
            return false;
         cl_log(" %u", page->actions[i].arguments[j]);
      }

      /* Double-linked list */
      action->prev_action = prev_action;
      action->next_action = NULL;
      if (prev_action)
         prev_action->next_action = action;
      
      cl_log("\n");
   }
   /* Zero-init the page's counters */
   memset(&page->counters, 0, sizeof(page->counters));
   
   cl_log("End of page.\n");

   return page->actions != 0;
}

bool cl_init_script(const char **pos)
{
   script.status = CL_SCRSTATUS_INACTIVE;

   if (!cl_strto(pos, &script.page_count, sizeof(script.page_count), false))
      return false;
   else
   {
      uint32_t i;

      script.pages = (cl_page_t*)calloc(script.page_count, sizeof(cl_page_t));
      for (i = 0; i < script.page_count; i++)
         if (!cl_init_page(pos, &script.pages[i]))
            return false;

      script.status = CL_SRCSTATUS_ACTIVE;
      return true;
   }
}

bool cl_is_if_statement(uint8_t type)
{
   return type > 0x60;
}

uint32_t cl_process_if_statements(cl_page_t *page, uint32_t pos)
{
   uint8_t  current_indent = page->actions[pos].indentation;
   bool     evaluation     = true;
   uint32_t i;

   for (i = pos; i < page->action_count; i++)
   {
      /* Make sure all if statements on the current indentation level are true */
      if (cl_is_if_statement(page->actions[i].type) && page->actions[i].indentation == current_indent)
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
            } while (page->actions[i].indentation > current_indent && i < page->action_count);
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
   uint32_t i       = 0;
   
   while (i < page->action_count)
   {
      script.current_action = &page->actions[i];

      if (script.status != CL_SRCSTATUS_ACTIVE)
         break;
      else if (cl_is_if_statement(page->actions[i].type))
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

bool cl_update_script()
{
   if (script.status != CL_SRCSTATUS_ACTIVE)
      return false;
   else
   {
      bool     success = true;
      uint32_t i;

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

   if (fatal)
      cl_fe_pause();

   va_start(args, format);
   vsprintf(script.error_msg, format, args);
   va_end(args);
}

#endif
