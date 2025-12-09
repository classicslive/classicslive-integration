#ifndef CL_NETWORK_H
#define CL_NETWORK_H

#include "cl_config.h"
#include "cl_types.h"

#if CL_HAVE_SSL
  #define CL_URL_SITE "https://" CL_URL_HOSTNAME
#else
  #define CL_URL_SITE "http://" CL_URL_HOSTNAME
#endif

#define CL_CLINT_URL CL_URL_SITE "/api/clint/v1/"

#define CL_END_CLINT_ACHIEVEMENT "achievement"
#define CL_END_CLINT_CLOSE "close"
#define CL_END_CLINT_LOGIN "login"
#define CL_END_CLINT_PING "ping"
#define CL_END_CLINT_START "start"
#define CL_END_CLINT_STOP "stop"

#if CL_HAVE_EDITOR
#define CL_API_URL CL_URL_SITE "/api/v1/"

#define CL_END_MEMORYNOTE_ADD "memory_note/add"
#define CL_END_SCRIPT_ADD "script/add"
#define CL_END_SCRIPT_EDIT "script/edit"
#endif

/* These are arbitrary and may need to be increased later. */
#define CL_POST_DATA_SIZE 2048
#define CL_URL_SIZE 256

#if CL_HAVE_EDITOR
void cl_network_post_api(const char *endpoint, const char *post_data,
  cl_network_cb_t callback, void *userdata);
#endif

void cl_network_post_clint(const char *endpoint, const char *post_data,
  cl_network_cb_t callback, void *userdata);

void cl_network_post_clint_login(const char *post_data,
  cl_network_cb_t callback);

#endif
