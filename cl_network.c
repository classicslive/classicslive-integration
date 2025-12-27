#include "cl_network.h"

#include "cl_common.h"
#include "cl_abi.h"
#include "cl_main.h"
#include "cl_memory.h"

#include <stdio.h>
#include <string.h>

static char *cl_build_generic_post_data(void)
{
  cl_counter_t value;
  unsigned bufsize = 256;
  char *buf = malloc(bufsize);
  char temp[128];
  unsigned i;

  if (!buf)
    return NULL;
  buf[0] = '\0';

  /* If logged in, add the session */
  if (session.state >= CL_SESSION_LOGGED_IN)
    snprintf(buf, bufsize, "session_id=%s", session.id);

  if (session.state == CL_SESSION_STARTED)
  {
    for (i = 0; i < memory.note_count; i++)
    {
      cl_memnote_t *note = &memory.notes[i];

      if (!cl_get_memnote_flag(note, CL_MEMFLAG_RICH))
        continue;
      if (cl_get_memnote_value(&value, note, CL_SRCTYPE_CURRENT_RAM))
      {
        unsigned written, need;

        if (cl_ctr_is_float(&note->current))
          written = snprintf(temp, sizeof(temp), "&m%u=%f",
                             note->key, value.floatval.fp);
        else
          written = snprintf(temp, sizeof(temp), "&m%u=%lli",
                             note->key, value.intval.i64);

        /* Grow buffer if needed */
        need = strlen(buf) + written + 1;
        if (need > bufsize)
        {
          bufsize *= 2;
          buf = realloc(buf, bufsize);
          if (!buf)
            return NULL;
        }

        strcat(buf, temp);
      }
    }
  }

  return buf;
}

static CL_NETWORK_CB(cl_default_network_cb)
{
  CL_UNUSED(userdata);
  cl_log(response.data);
}

static cl_error cl_network_post_internal(const char *url, const char *endpoint,
  const char *post_data, cl_network_cb_t callback, void *userdata, bool force)
{
  char endpoint_url[256];
  char final_data[1024];
  char *generic;

  if (session.state < CL_SESSION_LOGGED_IN && !force)
  {
    cl_message(CL_MSG_ERROR, "Tried to access CLint endpoint before login.");
    return CL_ERR_SESSION_MISMATCH;
  }
  else if (!url || !endpoint)
  {
    cl_message(CL_MSG_ERROR, "Unable to create request URL.");
    return CL_ERR_PARAMETER_NULL;
  }

  /* Create a general request */
  generic = cl_build_generic_post_data();
  if (!generic)
  {
    cl_log("Error building POST data");
    return CL_ERR_PARAMETER_NULL;
  }

  /* Add the request data, if given */
  if (post_data)
    snprintf(final_data, sizeof(final_data), "%s%s%s",
             post_data,
             generic[0] == '\0' ? "" : "&",
             generic);
  else
    snprintf(final_data, sizeof(final_data), "%s", generic);
  free(generic);

  /* Setup URL */
  snprintf(endpoint_url, sizeof(endpoint_url), "%s%s", url, endpoint);

  /* Give a default callback if none specified */
  if (!callback)
    callback = cl_default_network_cb;

  cl_abi_network_post(endpoint_url, final_data, callback, userdata);

  return CL_OK;
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
