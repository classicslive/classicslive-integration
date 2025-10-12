#ifndef CL_NETWORK_H
#define CL_NETWORK_H

#include "cl_config.h"
#include "cl_frontend.h"

#if CL_HAVE_SSL
  #define CL_URL_SITE "https://" CL_URL_HOSTNAME
#else
  #define CL_URL_SITE "http://" CL_URL_HOSTNAME
#endif

#define CL_CLINT_URL CL_URL_SITE "/api/clint/v1/"

#define CL_CLINT_ACHIEVEMENT "achievement"
#define CL_CLINT_CLOSE "close"
#define CL_CLINT_LOGIN "login"
#define CL_CLINT_PING "ping"
#define CL_CLINT_START "start"
#define CL_CLINT_STOP "stop"

/* These are arbitrary and may need to be increased later. */
#define CL_POST_DATA_SIZE 2048
#define CL_URL_SIZE       256

void cl_network_init(const char *new_session_id);
void cl_network_post(const char *request, const char *post_data, cl_network_cb_t callback);
void cl_network_discord();

#endif
