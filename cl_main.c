#ifndef CL_MAIN_C
#define CL_MAIN_C

#include <file/file_path.h>
#include <string/stdstring.h>

#include "cl_common.h"
#include "cl_identify.h"
#include "cl_json.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"
#include "frontend/cl_frontend.h"

#include "../../configuration.h"

/* Call C++ code only if the editor is built in */
#ifdef CL_HAVE_EDITOR
void cle_init();
void cle_run();
#endif

cl_session_t session;

bool cl_init_session(const char* json)
{
   const char *iterator;
   char session_id[CL_SESSION_ID_LENGTH];
   char memory_str[2048];
   char script_str[2048];

   cl_log("=====\nResponse from server:\n=====\n%s\n=====\n", json);

   /* Session-related */
   if (cl_json_get(session_id, json, "session_id", CL_JSON_STRING, sizeof(session_id)))
      cl_network_init(session_id);
   else
      return false;
   if (cl_json_get(&session.game_name, json, "title", CL_JSON_STRING, sizeof(session.game_name)))
      cl_message(CL_MSG_INFO, "Game name: %s\n", session.game_name);
   cl_json_get(&session.game_id, json, "game_id", CL_JSON_NUMBER, sizeof(session.game_id));

   /* Memory-related */
   iterator = &memory_str[0];
   if (!cl_json_get(memory_str, json, "memory_notes", CL_JSON_STRING, sizeof(memory_str)) || !cl_init_memory(&iterator))
      return false;
   cl_json_get(&memory.endianness,   json, "endianness",   CL_JSON_NUMBER, sizeof(memory.endianness));
   cl_json_get(&memory.pointer_size, json, "pointer_size", CL_JSON_NUMBER, sizeof(memory.pointer_size));
   cl_fe_install_membanks();
   session.ready = true;

   /* Script-related */
   iterator = &script_str[0];
   if (cl_json_get(script_str, json, "script", CL_JSON_STRING, sizeof(script_str)))
      cl_init_script(&iterator);
   else
      return false; // TODO

   return true;
}

static void cl_cb_login(struct retro_task *task, void *task_data, void *user_data, const char *error)
{
   char        msg[256];
   char       *server_data;
   const char *server_data_iterator;
   bool        success = false;
   http_transfer_data_t *response = (http_transfer_data_t*)task_data;
   
   if (error || !response->data)
      cl_log("CL: %s", error);
   else
   {
      if (!cl_json_get(&success, response->data, "success", CL_JSON_BOOLEAN, 0))
         cl_log("Malformed JSON output on login.\n");
      else if (!success)
      {
         char reason[256];

         if (cl_json_get(&reason, response->data, "reason", CL_JSON_STRING, 0))
            cl_message(CL_MSG_ERROR, reason);
         else
            cl_message(CL_MSG_ERROR, "Unknown error with login.");
      }
      else
      {
         success = cl_init_session(response->data);

      #ifdef CL_HAVE_EDITOR
         if (session.ready)
         {
            cl_update_memory();
            cle_init();
         }
      #endif
      }
   }
}

bool cl_post_empty_login()
{
   cl_message(CL_MSG_ERROR, "Please enter your password to sign in.");

   return false;
}

static void cl_post_login()
{
   settings_t *settings = config_get_ptr();
   char post_data[2048];

   snprintf
   (
      post_data, sizeof(post_data), 
      "request=login&hash=%.32s&username=%s&password=%s&filename=%s", 
      session.checksum,
      settings->CL_SETTINGS_USERNAME,
      settings->CL_SETTINGS_LOGIN_INFO,
      session.content_name
   );

   task_push_http_post_transfer(CL_REQUEST_URL, post_data, CL_TASK_MUTE, "POST", cl_cb_login, post_data);
}

bool cl_init(const void *data, const unsigned size, const char *path)
{
   settings_t *settings = config_get_ptr();

   cl_log("Init CL\n");
   /* If the user hasn't entered a username, they probably aren't using CL */
   if (!settings->CL_SETTINGS_USERNAME || string_is_empty(settings->CL_SETTINGS_USERNAME))
      return false;
   else if (!settings->CL_SETTINGS_LOGIN_INFO || string_is_empty(settings->CL_SETTINGS_LOGIN_INFO))
      return cl_post_empty_login();
   else
   {
      session.checksum[0]        = '\0';
      session.last_status_update = time(0);
      session.ready              = true;
      strncpy(session.content_name, path_basename(path), sizeof(session.content_name) - 1);

      cl_identify(data, size, path, session.checksum, cl_post_login);

      return true;
   }
}

bool cl_run()
{
   if (session.ready)
   {
      cl_update_memory();
      cl_update_script();

      /* Pingback every X seconds to update rich presence */
      if (time(0) >= session.last_status_update + CL_PRESENCE_INTERVAL)
      {
         session.last_status_update = time(0);
         cl_network_post(CL_REQUEST_PING, "", NULL, NULL);
      }

   #ifdef CL_HAVE_EDITOR
      cle_run();
   #endif

      return true;
   }

   return false;
}

void cl_free()
{
   cl_network_post(CL_REQUEST_CLOSE, NULL, NULL, NULL);
   cl_memory_free();
   cl_free_script(&script);
}

#endif
