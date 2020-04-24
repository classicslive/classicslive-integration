#ifndef CL_SCRIPT_C
#define CL_SCRIPT_C

#include "cl_script.h"

void cl_free_page(cl_page_t *page)
{
   uint32_t i;

   for (i = 0; i < page->action_count; i++)
      cl_free_action(&page->actions[i]);
}

void cl_free_script(cl_script_t *script)
{
   uint32_t i;

   for (i = 0; i < script->page_count; i++)
      cl_free_page(&script->pages[i]);
}

/*
uint32_t* cl_get_counter(uint8_t counter_num)
{
   if ((!current_page) || current_page->buffer.count - 1 < counter_num)
      return NULL;
   else
      return &current_page->buffer.memnotes[counter_num].value_current;
}

bool cl_get_counter_value(uint32_t *buffer, uint8_t counter_num)
{
   if ((!current_page) || current_page->buffer.count - 1 < counter_num)
      return false;
   else
      *buffer = current_page->buffer.memnotes[counter_num].value_current;

   return true;
}
*/

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
   /* TODO: Arbitrary allocation */
   memset(&page->counters, 0, sizeof(page->counters));
   cl_log("End of page.\n");

   return page->actions != 0;
}

bool cl_init_script(const char **pos, cl_script_t *script)
{
   if (!cl_strto(pos, &script->page_count, sizeof(script->page_count), false))
      return false;
   else
   {
      uint32_t i;

      script->pages = (cl_page_t*)calloc(script->page_count, sizeof(cl_page_t));
      for (i = 0; i < script->page_count; i++)
         if (!cl_init_page(pos, &script->pages[i]))
            return false;

      return true;
   }
}

bool cl_is_if_statement(uint8_t type)
{
   return type > 0x60;
}

uint32_t cl_process_if_statements(cl_page_t *page, cl_memory_t *memory, uint32_t pos)
{
   uint8_t  current_indent = page->actions[pos].indentation;
   bool     evaluation     = true;
   uint32_t i;

   for (i = pos; i < page->action_count; i++)
   {
      /* Make sure all if statements on the current indentation level are true */
      if (cl_is_if_statement(page->actions[i].type) && page->actions[i].indentation == current_indent)
         evaluation &= cl_process_action(&page->actions[i], memory);
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

bool cl_process_actions(cl_page_t *page, cl_memory_t *memory)
{
   bool     success = true;
   uint32_t i       = 0;
   
   while (i < page->action_count)
   {
      if (cl_is_if_statement(page->actions[i].type))
         i = cl_process_if_statements(page, memory, i);
      else
      {
         /* TODO: Error handling if a direct command returns false? */
         success &= cl_process_action(&page->actions[i], memory);
         i++;
      }
   }

   return success;
}

bool cl_update_script(cl_script_t *script, cl_memory_t *memory)
{
   bool     success = true;
   uint32_t i;

   for (i = 0; i < script->page_count; i++)
   {
      script->current_page = &script->pages[i];
      success &= cl_process_actions(script->current_page, memory);
   }

   return success;
}

#endif
