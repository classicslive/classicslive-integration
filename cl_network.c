#ifndef CL_NETWORK_C
#define CL_NETWORK_C

#include "cl_common.h"
/* For generic POST data */
#include "cl_memory.h"
#include "cl_network.h"

static char generic_post_data[CL_POST_DATA_SIZE];
static char session_id[CL_SESSION_ID_LENGTH];

bool cl_update_generic_post_data()
{
   bool     success = true;
   uint32_t value;
   uint32_t i;

   snprintf(generic_post_data, sizeof(generic_post_data), "session_id=%s", session_id);
   /*for (i = 0; i < cl_get_memnote_count(); i++)
   {
      success &= cl_get_memnote_value(&value, i, CL_SRCTYPE_CURRENT_RAM);
      if (success && cl_get_memnote_flag(i, CL_MEMFLAG_RICH))
         snprintf(generic_post_data, sizeof(generic_post_data), "%s&%u=%u", generic_post_data, i, value);
   }TODO*/

   return success;
}

void cl_network_init(const char *new_session_id)
{
   snprintf(session_id, sizeof(session_id), "%s", new_session_id);
}

void cl_network_post(const char *request, const char *post_data, retro_task_callback_t cb, void *user_data)
{
   if (string_is_empty(post_data))
      return;
   else
   {
      char new_post_data[CL_POST_DATA_SIZE];
   
      cl_update_generic_post_data();
      snprintf(new_post_data, CL_POST_DATA_SIZE, "request=%s&%s&%s", request, generic_post_data, post_data);
      new_post_data[CL_POST_DATA_SIZE - 1] = '\0';

      cl_log("cl_network_post:\nPOST: %s\n", new_post_data);
      task_push_http_post_transfer(CL_REQUEST_URL, new_post_data, CL_TASK_MUTE, "POST", cb, user_data);
   }
}

#endif
