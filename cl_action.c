#ifndef CL_ACTION_C
#define CL_ACTION_C

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

bool cl_get_compare_value(uint32_t *buffer, cl_memory_t *memory, 
   uint32_t source, uint32_t offset)
{
   if (!buffer)
      return false;
   else
   {
      bool success = true;

      switch (source)
      {
      case CL_SRCTYPE_IMMEDIATE:
         *buffer = offset;
         break;
      case CL_SRCTYPE_CURRENT_RAM:
         success = cl_get_memnote_value(buffer, memory, offset, CL_SRCTYPE_CURRENT_RAM);
         break;
      case CL_SRCTYPE_PREVIOUS_RAM:
         success = cl_get_memnote_value(buffer, memory, offset, CL_SRCTYPE_PREVIOUS_RAM);
         break;
      case CL_SRCTYPE_LAST_UNIQUE_RAM:
         success = cl_get_memnote_value(buffer, memory, offset, CL_SRCTYPE_LAST_UNIQUE_RAM);
         break;
      case CL_SRCTYPE_COUNTER:
         //success = cl_get_counter_value(buffer, offset);
         break;
      default:
         return false;
      }

      return success;
   }
}

static bool cl_act_post_achievement(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count != 1)
      return cl_free_action(action);
   else
   {
      uint32_t achievement_id;
      char     data[CL_POST_DATA_SIZE];

      achievement_id = action->arguments[0];
      snprintf(data, CL_POST_DATA_SIZE, "id=%u", achievement_id);
      cl_network_post(CL_REQUEST_POST_ACHIEVEMENT, data, NULL, NULL);
   }
   
   return true;
}

static bool cl_act_post_achievement_progress(cl_action_t *action, cl_memory_t *memory)
{
   /* TODO */
   return false;
}

static bool cl_act_post_leaderboard(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count % 2 != 1)
      return cl_free_action(action);
   else
   {
      char     data[CL_POST_DATA_SIZE];
      uint32_t value;
      uint8_t  i;

      snprintf(data, sizeof(data), "id=%u", action->arguments[0]);
      for (i = 1; i < action->argument_count; i += 2)
      {
         if (!cl_get_compare_value(&value, memory, action->arguments[i], action->arguments[i + 1]))
            continue;
         snprintf(data, sizeof(data), "%s&%u=%u", data, action->arguments[i + 1], value);
      }
      cl_network_post(CL_REQUEST_POST_LEADERBOARD, data, NULL, NULL);
   }
   
   return true;
}

static bool cl_act_compare(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count < 5)
      return cl_free_action(action);
   else
   {
      uint32_t left  = 0;
      uint32_t right = 0;

      if (!cl_get_compare_value(&left,  memory, action->arguments[0], action->arguments[1]) ||
          !cl_get_compare_value(&right, memory, action->arguments[2], action->arguments[3]))
         return cl_free_action(action);
      switch (action->arguments[4])
      {
      case CL_CMPTYPE_IFEQUAL:
         return left == right;
         break;
      case CL_CMPTYPE_IFGREATER:
         return left > right;
         break;
      case CL_CMPTYPE_IFLESS:
         return left < right;
         break;
      case CL_CMPTYPE_IFNEQUAL:
         return left != right;
         break;
      default:
         return cl_free_action(action);
      }
   }
}

static bool cl_act_hit_compare(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count != 7)
      return cl_free_action(action);
   else
   {
      if (cl_act_compare(action, memory))
         action->arguments[5]++;

      return (action->arguments[5] >= action->arguments[6]);
   }
}

static bool cl_act_no_process(cl_action_t *action, cl_memory_t *memory)
{
   return false;
}

static bool cl_act_changed(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count != 3)
      return cl_free_action(action);
   else
   {
      uint32_t left  = 0;
      uint32_t right = 0;

      if (!cl_get_compare_value(&left,  memory, CL_SRCTYPE_PREVIOUS_RAM, action->arguments[0]) ||
          !cl_get_compare_value(&right, memory, action->arguments[1], action->arguments[2]))
         return cl_free_action(action);

      return left == right;
   }
}

static bool cl_act_bits(cl_action_t *action, cl_memory_t *memory)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
      uint32_t value = 0;
      uint32_t bits  = 0;

      if (!cl_get_compare_value(&value, memory, action->arguments[0], action->arguments[1]) ||
          !cl_get_compare_value(&bits,  memory, action->arguments[2], action->arguments[3]))
         return cl_free_action(action);

      return (value & bits) == bits;
   }
}

bool cl_init_action(cl_action_t *action)
{
   action->function = NULL;

   switch (action->type)
   {
   case CL_ACTTYPE_NO_PROCESS:
      action->function = cl_act_no_process;
      break;
   case CL_ACTTYPE_CHANGED:
      action->function = cl_act_changed;
      break;
   case CL_ACTTYPE_COMPARE:
      action->function = cl_act_compare;
      break;
   case CL_ACTTYPE_BITS:
      action->function = cl_act_bits;
      break;
   case CL_ACTTYPE_HIT_COMPARE:
      action->function = cl_act_hit_compare;
      break;
   case CL_ACTTYPE_POST_ACHIEVEMENT:
      action->function = cl_act_post_achievement;
      break;
   case CL_ACTTYPE_POST_LEADERBOARD:
      action->function = cl_act_post_leaderboard;
      break;
   case CL_ACTTYPE_POST_PROGRESS:
      action->function = cl_act_post_achievement_progress;
      break;
   }

   return action->function != NULL;
}

/* We assume the function pointer is not NULL */
bool cl_process_action(cl_action_t *action, cl_memory_t *memory)
{
   return action->function(action, memory);
}

#endif