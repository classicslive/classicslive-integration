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

/* Call C++ code only if the editor is built in */
#ifdef CL_HAVE_EDITOR
void cle_init();
void cle_run();
#endif

cl_session_t session;
static cl_user_t user;

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
      cl_script_init(&iterator);
   else
      return false; // TODO

   return true;
}

static void cl_cb_login(cl_network_response_t response)
{
   bool success = false;
   
   if (response.error_code || !response.data)
      cl_log("Network error on login: %u (%s)\n", 
         response.error_code, 
         response.error_msg);
   else
   {
      if (!cl_json_get(&success, response.data, "success", CL_JSON_BOOLEAN, 0))
         cl_log("Malformed JSON output on login.\n");
      else if (!success)
      {
         char reason[256];

         if (cl_json_get(&reason, response.data, "reason", CL_JSON_STRING, 0))
            cl_message(CL_MSG_ERROR, reason);
         else
            cl_message(CL_MSG_ERROR, "Unknown error with login.");
      }
      else
      {
         success = cl_init_session(response.data);

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
   char post_data[2048];

   snprintf
   (
      post_data, sizeof(post_data), 
      "hash=%.32s&username=%s&password=%s&filename=%s%s%s", 
      session.checksum,
      user.username,
      user.password,
      session.content_name,
      user.language ? "&lang=" : "",
      user.language ? user.language : ""
   );

   cl_network_post(CL_REQUEST_LOGIN, post_data, cl_cb_login);
}

bool cl_init(const void *data, const unsigned size, const char *path)
{
   cl_log("Init CL\n");

   /* Retrieve user login info */
   cl_fe_user_data(&user, 0);

   /* If the user hasn't entered a username, they probably aren't using CL */
   if (!user.username || string_is_empty(user.username))
      return false;
   else if (!user.password || string_is_empty(user.password))
      return cl_post_empty_login();
   else
   {
      /* Init session values */
      session.checksum[0]        = '\0';
      session.last_status_update = time(0);
      session.ready              = true;
      strncpy(session.content_name, path_basename(path), 
         sizeof(session.content_name) - 1);

      /* Pass information off to content identification code */
      cl_identify(data, size, path, cl_fe_library_name(), session.checksum,
         cl_post_login);

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
         cl_network_post(CL_REQUEST_PING, "", NULL);
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
   cl_network_post(CL_REQUEST_CLOSE, "", NULL);
   cl_memory_free();
   cl_script_free();
}
