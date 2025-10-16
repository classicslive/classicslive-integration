#ifndef CL_MAIN_H
#define CL_MAIN_H

#include "cl_types.h"

cl_error cl_login(void);

cl_error cl_start(cl_game_identifier_t identifier);

cl_error cl_login_and_start(cl_game_identifier_t identifier);

cl_error cl_run(void);

void cl_free(void);

extern cl_session_t session;

#endif
