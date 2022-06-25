#ifndef CL_FRONTEND_H
#define CL_FRONTEND_H

typedef struct cl_task_t
{
   void  *state;
   void (*handler)(struct cl_task_t*);
   void (*callback)(struct cl_task_t*);
   char  *error;
} cl_task_t;

/**
 * @file cl_frontend.h
 * The functions prototyped below are implementation-specific callbacks that
 * must be written for any program intended to be used with Classics Live. 
 **/

/**
 * Signals to the frontend to display a message.
 * @param level The severity of the message. For example, CL_MSG_WARNING.
 * @param msg The message to be displayed.
 **/
void cl_fe_display_message(unsigned level, const char *msg);

/**
 * Signals to the frontend to write memory bank data into the global "memory"
 * struct.
 **/
bool cl_fe_install_membanks(void);

/**
 * Sends an HTTP POST request.
 * For internal use. cl_network_post should be used instead.
 **/
void cl_fe_network_post(const char *url, const char *data, void *callback);

/**
 * Signals to the frontend to stop processing new frames until unpaused.
 **/
void cl_fe_pause(void);

/**
 * Instructs the frontend to spin a function into a seperate thread.
 **/
void cl_fe_thread(cl_task_t *task);

/**
 * Signals to the frontend to resume processing new frames.
 **/
void cl_fe_unpause(void);

#endif
