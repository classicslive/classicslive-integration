#include "cl_common.h"
#include "cl_frontend.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"

#include <stdio.h>

static char generic_post_data[CL_POST_DATA_SIZE];

static cl_error cl_update_generic_post_data(void)
{
  cl_counter_t value;
  unsigned i;

  if (session.state >= CL_SESSION_LOGGED_IN)
    snprintf(generic_post_data, sizeof(generic_post_data), "session_id=%s",
             session.id);

  if (session.state == CL_SESSION_STARTED)
  {
    /* Track the values of notes that have the "rich" flag set. */
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
            snprintf(generic_post_data, sizeof(generic_post_data), "%s&m%u=%li",
              generic_post_data, note->key, value.intval.i64);
        }
        else
          return CL_ERR_CLIENT_RUNTIME;
      }
    }
  }

  return CL_OK;
}

static CL_NETWORK_CB(cl_default_network_cb)
{
  CL_UNUSED(userdata);
  cl_log(response.data);
}

static void cl_network_post_internal(const char *url, const char *endpoint,
  const char *post_data, cl_network_cb_t callback, void *userdata, bool force)
{
  if (session.state < CL_SESSION_LOGGED_IN && !force)
  {
    cl_message(CL_MSG_ERROR, "Tried to access CLint endpoint before login.");
    return;
  }
  else if (!url || !endpoint)
  {
    cl_message(CL_MSG_ERROR, "Unable to create request URL.");
    return;
  }
  else
  {
    char endpoint_url[256];

    cl_update_generic_post_data();
    /* Add additional POST data if given */
    if (post_data)
      snprintf(generic_post_data, sizeof(generic_post_data), "%s&%s",
        generic_post_data, post_data);
    cl_log("cl_network_post:\nPOST: %s\n", generic_post_data);

    /* Apply default callback if none is specified */
    if (!callback)
      callback = cl_default_network_cb;

    /* Create the full endpoint URL */
    snprintf(endpoint_url, sizeof(endpoint_url), "%s%s", url, endpoint);

    cl_fe_network_post(endpoint_url,
                       generic_post_data,
                       callback ? callback : cl_default_network_cb,
                       userdata);
  }
}

#if CL_HAVE_EDITOR
void cl_network_post_api(const char *endpoint, const char *post_data,
  cl_network_cb_t callback, void *userdata)
{
  cl_network_post_internal(CL_API_URL, endpoint,
                           post_data, callback, userdata, false);
}
#endif

void cl_network_post_clint(const char *endpoint, const char *post_data,
  cl_network_cb_t callback, void *userdata)
{
  cl_network_post_internal(CL_CLINT_URL, endpoint,
                           post_data, callback, userdata, false);
}

void cl_network_post_clint_login(const char *post_data,
  cl_network_cb_t callback)
{
  cl_network_post_internal(CL_CLINT_URL, CL_END_CLINT_LOGIN,
                           post_data, callback, NULL, true);
}
