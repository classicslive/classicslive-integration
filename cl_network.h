#ifndef CL_NETWORK_H
#define CL_NETWORK_H

#include "cl_frontend.h"

#ifdef HAVE_SSL
#define CL_URL_SITE "http://192.168.86.10/"
#else
#define CL_URL_SITE "http://192.168.86.10/"
#endif

#define CL_REQUEST_URL              "http://192.168.86.10/classicslive/public/api/request.php"

#define CL_REQUEST_LOGIN            "login"
#define CL_REQUEST_ADD_MEMNOTE      "add_memory_note"
#define CL_REQUEST_CLOSE            "close"
#define CL_REQUEST_PING             "ping"
#define CL_REQUEST_POST_ACHIEVEMENT "post_unlock"
#define CL_REQUEST_POST_HASH        "post_hash"
#define CL_REQUEST_POST_LEADERBOARD "post_lb_entry"
#define CL_REQUEST_POST_PRESENCE    "post_presence"
#define CL_REQUEST_POST_PROGRESS    "post_progress"

/* These are arbitrary and may need to be increased later. */
#define CL_POST_DATA_SIZE 2048
#define CL_URL_SIZE       256

void cl_network_init(const char *new_session_id);
void cl_network_post(const char *request, const char *post_data, cl_network_cb_t callback);
void cl_network_discord();

#endif
