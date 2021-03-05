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

bool cl_get_compare_value(uint32_t *buffer, uint32_t source, uint32_t offset)
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
      case CL_SRCTYPE_PREVIOUS_RAM:
      case CL_SRCTYPE_LAST_UNIQUE_RAM:
         success = cl_get_memnote_value_from_key(buffer, offset, source);
         break;
      case CL_SRCTYPE_COUNTER:
         success = cl_get_counter_value(buffer, offset);
         break;
      default:
         return false;
      }

      return success;
   }
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
      cl_network_post(CL_REQUEST_POST_ACHIEVEMENT, data, NULL, NULL);

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
      cl_network_post(CL_REQUEST_POST_PROGRESS, data, NULL, NULL);
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
      cl_network_post(CL_REQUEST_POST_LEADERBOARD, data, NULL, NULL);
   }
   
   return true;
}

static bool cl_act_compare(cl_action_t *action)
{
   if (action->argument_count < 5)
      return cl_free_action(action);
   else
   {
      uint32_t left  = 0;
      uint32_t right = 0;

      if (!cl_get_compare_value(&left,  action->arguments[0], action->arguments[1]) ||
          !cl_get_compare_value(&right, action->arguments[2], action->arguments[3]))
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

static bool cl_act_hit_compare(cl_action_t *action)
{
   if (action->argument_count != 7)
      return cl_free_action(action);
   else
   {
      if (cl_act_compare(action))
         action->arguments[5]++;

      return (action->arguments[5] >= action->arguments[6]);
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
      uint32_t left  = 0;
      uint32_t right = 0;

      if (!cl_get_compare_value(&left,  CL_SRCTYPE_PREVIOUS_RAM, action->arguments[0]) ||
          !cl_get_compare_value(&right, action->arguments[1], action->arguments[2]))
         return cl_free_action(action);

      return left == right;
   }
}

static bool cl_act_bits(cl_action_t *action)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
      uint32_t value = 0;
      uint32_t bits  = 0;

      if (!cl_get_compare_value(&value, action->arguments[0], action->arguments[1]) ||
          !cl_get_compare_value(&bits,  action->arguments[2], action->arguments[3]))
         return cl_free_action(action);

      return (value & bits) == bits;
   }
}

static bool cl_act_write(cl_action_t *action)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
      uint32_t src;

      if (!cl_get_compare_value(&src, action->arguments[2], action->arguments[3]))
         return false;
      else
      {
         switch (action->arguments[0])
         {
         case CL_SRCTYPE_CURRENT_RAM:
            return cl_write_memorynote(action->arguments[1], &src);
         case CL_SRCTYPE_COUNTER:
            script.current_page->counters[action->arguments[1]] = src;
            break;
         default:
            return false;
         }
      }
      
      return true;
   }
}

static bool cl_act_addition(cl_action_t *action)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
      uint32_t src_type = action->arguments[0];
      uint32_t src_val  = action->arguments[1];
      uint32_t add_type = action->arguments[2];
      uint32_t add_val  = action->arguments[3];
      uint32_t src, add;

      if (!cl_get_compare_value(&src, src_type, src_val) ||
          !cl_get_compare_value(&add, add_type, add_val))
         return false;
      else
      {
         switch (src_type)
         {
         case CL_SRCTYPE_COUNTER:
            script.current_page->counters[src_val] += add;
            break;
         default:
            return false;
         }
      }

      return true;
   }
}

static bool cl_act_multiplication(cl_action_t *action)
{
   if (action->argument_count % 2 || action->argument_count < 4)
      return cl_free_action(action);
   else
   {
      uint32_t dest_type = action->arguments[0];
      uint32_t dest_val  = action->arguments[1];
      uint32_t result;
      uint8_t i;

      /* Make sure requested destination is valid */
      if (!cl_get_compare_value(&result, dest_type, dest_val))
         return false;

      /* Make sure requested multipliers are valid, and multiply if so */
      for (i = 0; i < action->argument_count - 2; i += 2)
      {
         uint32_t type   = action->arguments[i];
         uint32_t offset = action->arguments[i + 1];
         uint32_t value;

         if (!cl_get_compare_value(&value, type, offset))
            return false;
         result *= value;
      }

      switch (dest_type)
      {
      case CL_SRCTYPE_COUNTER:
         script.current_page->counters[dest_val] = result;
         break;
      default:
         return false;
      }

      return true;
   }
}

static bool cl_act_subtraction(cl_action_t *action)
{
   if (action->argument_count != 4)
      return cl_free_action(action);
   else
   {
      uint32_t src_type = action->arguments[0];
      uint32_t src_val  = action->arguments[1];
      uint32_t sub_type = action->arguments[2];
      uint32_t sub_val  = action->arguments[3];
      uint32_t src, sub;

      if (!cl_get_compare_value(&src, src_type, src_val) ||
          !cl_get_compare_value(&sub, sub_type, sub_val))
         return false;
      else
      {
         uint32_t result = src - sub;
         bool return_val = result < src;

         switch (src_type)
         {
         case CL_SRCTYPE_COUNTER:
            script.current_page->counters[src_val] = result;
            break;
         default:
            return false;
         }

         /* TODO: This is false on underflow. Should we do anything else? */
         return return_val;
      }
   }
}

bool cl_init_action(cl_action_t *action)
{
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
   case CL_ACTTYPE_ADDITION:
      action->function = cl_act_addition;
      break;
   case CL_ACTTYPE_POST_LEADERBOARD:
      action->function = cl_act_post_leaderboard;
      break;
   case CL_ACTTYPE_MULTIPLY:
      action->function = cl_act_multiplication;
      break;
   case CL_ACTTYPE_POST_PROGRESS:
      action->function = cl_act_post_achievement_progress;
      break;
   case CL_ACTTYPE_WRITE:
      action->function = cl_act_write;
      break;
   default:
      action->function = cl_act_no_process;
      return false;
   }

   return true;
}

bool cl_process_action(cl_action_t *action)
{
   if (action && action->function && action->function(action))
   {
      action->executions++;
      return true;
   }
   
   return false;
}

#endif
