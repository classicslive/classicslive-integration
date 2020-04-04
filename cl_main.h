#ifndef CL_MAIN_H
#define CL_MAIN_H

#include "cl_types.h"

bool cl_init(struct retro_game_info *info);
bool cl_run();

extern cl_session_t session;

#endif
