#include <command.h>

#include "cl_frontend.h"

void cl_fe_pause(void)
{
   command_event(CMD_EVENT_PAUSE, NULL);
}

void cl_fe_unpause(void)
{
   command_event(CMD_EVENT_UNPAUSE, NULL);
}
