#ifndef CL_FRONTEND_H
#define CL_FRONTEND_H

#include "cl_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
 * Has the frontend return a string representing the name of the program that
 * is having its memory inspected.
 * This could be the name of a dynamically linked library, an external process
 * name, or the name of a program CL has been statically compiled into.
 * @return The library name, or NULL if unavailable.
 **/
const char* cl_fe_library_name(void);

/**
 * Instructs the frontend to send an HTTP POST request. Implementation of this
 * function needs to explicitly free the POST data.
 * For internal use. User code should call cl_network_post instead.
 **/
void cl_fe_network_post(const char *url, char *data,
   void(*callback)(cl_network_response_t));

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

/**
 * Requests known user data from the frontend, used for retrieving login
 * information and settings data.
 * @param user A pointer to the user data being written to.
 * @param index The user index being requested; ie. P1, P2, P3. Zero-indexed.
 **/
bool cl_fe_user_data(cl_user_t *user, unsigned index);

/**
 * These frontend functions are only used for frontends that nned to interface
 * with external memory.
 **/
#if CL_EXTERNAL_MEMORY == true
#include <cl_memory.h>
#include <cl_search.h>

bool cl_fe_memory_read(cl_memory_t *memory, void *dest, cl_addr_t address,
   unsigned size);

bool cl_fe_memory_write(cl_memory_t *memory, void *src, cl_addr_t address,
   unsigned size);

/**
 * Requests a deep copy of host memory into a search struct.
 **/
bool cl_fe_search_deep_copy(cl_search_t *search);
#endif

#ifdef __cplusplus
}
#endif

#endif
