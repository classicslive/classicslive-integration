#ifndef CL_MAIN_C
#define CL_MAIN_C

#include <stdio.h>

#define CL_HAVE_EDITOR true

#include "cl_common.h"
#include "cl_identify.h"
#include "cl_json.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"

/* Call C++ code only if the editor is built in */
#ifdef CL_HAVE_EDITOR
void cle_init();
void cle_run();
#endif

static cl_memory_t  memory;
static cl_script_t  script;
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
      cl_error("Game name: %s\n", session.game_name);
   cl_json_get(&session.game_id, json, "game_id", CL_JSON_NUMBER, sizeof(session.game_id));

   /* Memory-related */
   iterator = &memory_str[0];
   if (cl_json_get(memory_str, json, "memory_notes", CL_JSON_STRING, sizeof(memory_str)))
      cl_init_memory(&iterator, &memory);
   else
      return false;
   cl_json_get(&memory.endianness,   json, "endianness",   CL_JSON_NUMBER, sizeof(memory.endianness));
   cl_json_get(&memory.pointer_size, json, "pointer_size", CL_JSON_NUMBER, sizeof(memory.pointer_size));

   /* Script-related */
   iterator = &script_str[0];
   if (cl_json_get(script_str, json, "script", CL_JSON_STRING, sizeof(script_str)))
      cl_init_script(&iterator, &script);
   else
      return false;

   session.ready = true;

   return true;
}

static void cl_cb_login(struct retro_task *task, void *task_data, void *user_data, const char *error)
{
   char        msg[256];
   char       *server_data;
   const char *server_data_iterator;
   http_transfer_data_t *response = (http_transfer_data_t*)task_data;
   
   if (error || !response->data)
      cl_log("CL: %s", error);
   else
   {
      bool success;

      if (!cl_json_get(&success, response->data, "success", CL_JSON_BOOLEAN, 0))
         cl_log("Malformed JSON output on login.\n");
      else if (!success)
      {
         char reason[256];

         if (cl_json_get(&reason, response->data, "reason", CL_JSON_STRING, 0))
            cl_error(reason);
         else
            cl_error("Unknown error with login.");
      }
      else
         cl_init_session(response->data);
   }
}

bool cl_post_empty_login()
{
   cl_error("Please enter your password to sign in.");

   return false;
}

static void cl_post_login()
{
   char post_data[2048];

   snprintf(post_data, sizeof(post_data), 
      "request=login&hash=%.32s&username=Celery&filename=%s", 
      session.checksum, session.content_name);
   post_data[2047] = '\0';

   task_push_http_post_transfer(CL_REQUEST_URL, post_data, CL_TASK_MUTE, "POST", cl_cb_login, post_data);
}

bool cl_init(struct retro_game_info *info)
{
   settings_t *settings = config_get_ptr();

#ifdef CL_HAVE_EDITOR
   cle_init(&memory);
#endif

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
      session.ready              = false;
      strncpy(session.content_name, path_basename(info->path), sizeof(session.content_name) - 1);

      cl_identify(info, session.checksum, cl_post_login);

      return true;
   }
}

bool cl_run()
{
   if (true)//session.ready) TODO: temp
   {
      cl_update_memory(&memory);
      cl_update_script(&script, &memory);
#ifdef CL_HAVE_EDITOR
      cle_run();
#endif

      return true;
   }

   return false;
}

void cl_free()
{
   cl_free_memory(&memory);
   cl_free_script(&script);
}

#endif
