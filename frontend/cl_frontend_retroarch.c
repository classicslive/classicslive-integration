#include <command.h>

#include "cl_frontend.h"

void cl_fe_display_error(const char *msg)
{
   runloop_msg_queue_push(msg, 0, 2 * 60, false, NULL, 0, 0);
}

void cl_fe_pause(void)
{
   command_event(CMD_EVENT_PAUSE, NULL);
}

void cl_fe_unpause(void)
{
   command_event(CMD_EVENT_UNPAUSE, NULL);
}
