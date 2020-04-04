#ifndef CL_NETWORK_H
#define CL_NETWORK_H

#ifdef HAVE_SSL
#define CL_URL_SITE "http://127.0.0.1/"
#else
#define CL_URL_SITE "http://127.0.0.1/"
#endif

#define CL_REQUEST_URL              "http://127.0.0.1/classicslive/public/api/request.php"

#define CL_REQUEST_LOGIN            "login"
#define CL_REQUEST_ADD_MEMNOTE      "add_memory_note"
#define CL_REQUEST_POST_ACHIEVEMENT "post_unlock"
#define CL_REQUEST_POST_HASH        "post_hash"
#define CL_REQUEST_POST_LEADERBOARD "post_lb_entry"
#define CL_REQUEST_POST_PRESENCE    "post_presence"
#define CL_REQUEST_POST_PROGRESS    "post_progress"

/* These are arbitrary and may need to be increased later. */
#define CL_POST_DATA_SIZE 2048
#define CL_URL_SIZE       256

#include "../../discord/discord.h"
#include "../../network/net_http_special.h"
#include "../../tasks/tasks_internal.h"

void cl_network_init(const char *new_session_id);
void cl_network_post(const char *request, const char *post_data, retro_task_callback_t cb, void *user_data);
void cl_network_discord();

#endif