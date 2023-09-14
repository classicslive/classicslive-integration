#ifndef CL_MAIN_H
#define CL_MAIN_H

#include "cl_types.h"

bool cl_init(const void *data, const unsigned size, const char *info_path);

bool cl_run(void);

void cl_free(void);

extern cl_session_t session;

#endif
