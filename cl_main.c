#include <file/file_path.h>
#include <string/stdstring.h>

#include "cl_common.h"
#include "cl_frontend.h"
#include "cl_identify.h"
#include "cl_json.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"

/* Call C++ code only if the editor is built in */
#if CL_HAVE_EDITOR
void cle_init(void);
void cle_run(void);
#endif

cl_session_t session;

static cl_error cl_init_session(const char* json)
{
  const char *iterator;
  char script_str[2048];
  unsigned misc, i;

  /* Get game info */
  if (cl_json_get(&session.game_title, json, CL_JSON_KEY_TITLE,
                  CL_JSON_TYPE_STRING, sizeof(session.game_title)))
    cl_message(CL_MSG_INFO, "Game title: %s\n", session.game_title);
  if (cl_json_get(&misc, json, CL_JSON_KEY_GAME_ID,
                  CL_JSON_TYPE_NUMBER, sizeof(misc)))
    session.game_id = misc;

  /* Get default endianness of memory regions */
  if (cl_json_get(&misc, json, CL_JSON_KEY_ENDIANNESS,
                  CL_JSON_TYPE_NUMBER, sizeof(misc)))
    for (i = 0; i < memory.region_count; i++)
      memory.regions[i].endianness = misc;

  /* Get default pointer length of memory regions */
  if (cl_json_get(&misc, json, CL_JSON_KEY_POINTER_SIZE,
                  CL_JSON_TYPE_NUMBER, sizeof(misc)))
    for (i = 0; i < memory.region_count; i++)
      memory.regions[i].pointer_length = misc;

  if (!cl_fe_install_membanks())
    return CL_ERR_CLIENT_RUNTIME;

  /* Get memory notes */
  cl_json_get_array((void**)&memory.notes, &memory.note_count,
    json, CL_JSON_KEY_MEMORY_NOTES, CL_JSON_TYPE_MEMORY_NOTE);
  cl_memory_init_notes();

  /* Get script */
  iterator = &script_str[0];
  if (cl_json_get(script_str, json, CL_JSON_KEY_SCRIPT, CL_JSON_TYPE_STRING, sizeof(script_str)))
  {
    if (!cl_script_init(&iterator))
    {
      cl_message(CL_MSG_ERROR, "Failed to initialize CL script.");
#if !CL_HAVE_EDITOR
      return CL_ERR_SERVER_UNEXPECTED_RESPONSE;
#endif
    }

    cl_json_get_array((void**)&session.achievements, &session.achievement_count,
      json, CL_JSON_KEY_ACHIEVEMENTS, CL_JSON_TYPE_ACHIEVEMENT);

    cl_json_get_array((void**)&session.leaderboards, &session.leaderboard_count,
      json, CL_JSON_KEY_LEADERBOARDS, CL_JSON_TYPE_LEADERBOARD);
  }
#if !CL_HAVE_EDITOR
  else
    return CL_ERR_SERVER_UNEXPECTED_RESPONSE;
#endif

  return CL_OK;
}

static CL_NETWORK_CB(cl_start_cb)
{
  CL_UNUSED(userdata);
  if (response.error_code || cl_init_session(response.data) != CL_OK)
  {
    session.state = CL_SESSION_LOGGED_IN;
    return;
  }
  else
  {
    session.state = CL_SESSION_STARTED;
#if CL_HAVE_EDITOR
    cl_update_memory();
    cle_init();
#endif
  }
}

cl_error cl_start(cl_game_identifier_t identifier)
{
  if (session.state < CL_SESSION_LOGGED_IN)
    return CL_ERR_CLIENT_RUNTIME;
  else
  {
    char post_data[CL_POST_DATA_SIZE];

    post_data[0] = '\0';
    snprintf(post_data, sizeof(post_data),
             "session_id=%s&library=%s&filename=%s",
             session.id, cl_fe_library_name(), session.content_name);

    if (identifier.type == CL_GAMEIDENTIFIER_FILE_HASH)
    {
      if (strlen(identifier.checksum) == 32)
      {
        strcat(post_data, "&md5=");
        strcat(post_data, identifier.checksum);
      }
      else
      {
        cl_message(CL_MSG_ERROR, "Invalid identification checksum.");
        return CL_ERR_CLIENT_RUNTIME;
      }
    }
    else if (identifier.type == CL_GAMEIDENTIFIER_PRODUCT_CODE)
    {
      int success = 0;

      if (strlen(identifier.product) > 0)
      {
        strcat(post_data, "&product=");
        strcat(post_data, identifier.product);
        success = 1;
      }
      if (strlen(identifier.version) > 0)
      {
        strcat(post_data, "&version=");
        strcat(post_data, identifier.version);
        success = 1;
      }
      if (!success)
      {
        cl_message(CL_MSG_ERROR, "No product code information passed.");
        return CL_ERR_CLIENT_RUNTIME;
      }
    }
    session.state = CL_SESSION_STARTING;
    cl_network_post_clint(CL_END_CLINT_START, post_data, cl_start_cb, NULL);

    return CL_OK;
  }
}

static CL_NETWORK_CB(cl_login_cb)
{
  CL_UNUSED(userdata);
  if (response.error_code)
    session.state = CL_SESSION_NONE;
  else
  {
    unsigned char success;

    if (!cl_json_get(&success, response.data, CL_JSON_KEY_SUCCESS,
        CL_JSON_TYPE_BOOLEAN, 0))
    {
      cl_log("Malformed JSON output on login.\n%s", response.data);
      return;
    }
    else if (!success)
      return;

    if (cl_json_get(session.id, response.data, CL_JSON_KEY_SESSION_ID,
        CL_JSON_TYPE_STRING, sizeof(session.id)))
      session.state = CL_SESSION_LOGGED_IN;
  }
}

static cl_error cl_login_internal(cl_network_cb_t callback)
{
  cl_user_t user;
  char post_data[256];

  /* Retrieve user login info */
  if (!cl_fe_user_data(&user, 0))
  {
    cl_message(CL_MSG_ERROR, "Unable to retreive CL login data.");
    return CL_ERR_USER_CONFIG;
  }

  /**
   * Add login info; username is required, one of either token or password
   * must be present. Token is preferred.
   */
  if (!strlen(user.username))
    goto login_error;
  else if (strlen(user.token))
    snprintf(post_data, sizeof(post_data), "username=%s&token_clint=%s",
      user.username, user.token);
  else if (strlen(user.password))
    snprintf(post_data, sizeof(post_data), "username=%s&password=%s",
      user.username, user.password);
  else
    goto login_error;

  /* Append editor flag if available */
#if CL_HAVE_EDITOR
  strcat(post_data, "&editor=true");
#endif

  session.state = CL_SESSION_LOGGING_IN;
  cl_network_post_clint_login(post_data, callback);

  return CL_OK;

login_error:
  cl_message(CL_MSG_ERROR, "Unable to retreive CL login data.\n"
    "Please enter your username and password.");
  return CL_ERR_USER_CONFIG;
}

static CL_NETWORK_CB(cl_login_and_start_cb_2)
{
  if (response.error_code)
    return;
  else
  {
    cl_login_cb(response, userdata);
    cl_start(session.identifier);
  }
}

static CL_TASK_CB(cl_login_and_start_cb_1)
{
  if (!task->error)
    cl_login_internal(cl_login_and_start_cb_2);
}

cl_error cl_login(void)
{
  if (session.state == CL_SESSION_NONE)
    return cl_login_internal(cl_login_cb);
  else
  {
    cl_message(CL_MSG_ERROR, "cl_login state mismatch");
    return CL_ERR_CLIENT_RUNTIME;
  }
}

cl_error cl_login_and_start(cl_game_identifier_t identifier)
{
  if (session.state == CL_SESSION_NONE)
  {
    session.identifier = identifier;
#if CL_HAVE_FILESYSTEM
    strncpy(session.content_name, path_basename(identifier.filename),
            sizeof(session.content_name) - 1);
#endif
    cl_identify(identifier.data, identifier.size, identifier.filename,
                cl_fe_library_name(), session.identifier.checksum,
                cl_login_and_start_cb_1);
    return CL_OK;
  }
  else
  {
    cl_message(CL_MSG_ERROR, "cl_login_and_start state mismatch");
    return CL_ERR_CLIENT_RUNTIME;
  }
}

cl_error cl_run(void)
{
  if (session.state == CL_SESSION_STARTED)
  {
    cl_update_memory();
    cl_script_update();

    /* Pingback every X seconds to update rich presence */
    if (time(0) >= session.last_status_update + CL_PRESENCE_INTERVAL)
    {
      session.last_status_update = time(0);
      cl_network_post_clint(CL_END_CLINT_PING, NULL, NULL, NULL);
    }

#if CL_HAVE_EDITOR
    cle_run();
#endif

    return CL_OK;
  }

  return CL_ERR_CLIENT_RUNTIME;
}

cl_error cl_free(void)
{
  if (session.state >= CL_SESSION_LOGGED_IN)
    cl_network_post_clint(CL_END_CLINT_CLOSE, NULL, NULL, NULL);
  cl_memory_free();
  cl_script_free();
  memset(&session, 0, sizeof(session));

  return CL_OK;
}
