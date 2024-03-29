#include "cl_common.h"
#include "cl_frontend.h"
#include "cl_memory.h"
#include "cl_network.h"

static char generic_post_data[CL_POST_DATA_SIZE];
static char session_id[CL_SESSION_ID_LENGTH];
static bool logged_in = false;

bool cl_update_generic_post_data()
{
  cl_counter_t value;
  unsigned i;

  snprintf(generic_post_data, sizeof(generic_post_data), "session_id=%s",
    session_id);

  /*
    Track the values of all memory notes that have the "rich presence"
    flag set.
  */
  for (i = 0; i < memory.note_count; i++)
  {
    cl_memnote_t *note = &memory.notes[i];

    if (cl_get_memnote_flag(note, CL_MEMFLAG_RICH))
    {
      if (cl_get_memnote_value(&value, note, CL_SRCTYPE_CURRENT_RAM))
      {
        if (cl_ctr_is_float(&note->current))
          snprintf(generic_post_data, sizeof(generic_post_data), "%s&m%u=%f",
            generic_post_data, note->key, value.floatval.fp);
        else
          snprintf(generic_post_data, sizeof(generic_post_data), "%s&m%u=%lli",
            generic_post_data, note->key, value.intval.i64);
      }
      else
        return false;
    }
  }

  return true;
}

void cl_network_init(const char *new_session_id)
{
  snprintf(session_id, sizeof(session_id), "%s", new_session_id);
  session_id[CL_SESSION_ID_LENGTH - 1] = '\0';
  logged_in = true;
}

void cl_default_network_cb(cl_network_response_t response)
{
  cl_log(response.data);
}

void cl_network_post(const char *request, const char *post_data,
  cl_network_cb_t callback)
{
  char *new_post_data = calloc(CL_POST_DATA_SIZE, sizeof(char));
   
  if (logged_in)
    cl_update_generic_post_data();
  snprintf(new_post_data, CL_POST_DATA_SIZE, "request=%s&%s&%s",
    request, generic_post_data, post_data ? post_data : "");
  cl_log("cl_network_post:\nPOST: %s\n", new_post_data);

  if (!callback)
    callback = cl_default_network_cb;
   
  cl_fe_network_post(CL_REQUEST_URL, new_post_data, callback);
}
