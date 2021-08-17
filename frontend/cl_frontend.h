#ifndef CL_FRONTEND_H
#define CL_FRONTEND_H

/**
 * @file cl_frontend.h
 * The functions prototyped below are implementation-specific callbacks that
 * must be written for any program intended to be used with Classics Live. 
 **/

/**
 * Signals to the frontend to stop processing new frames until unpaused.
 **/
void cl_fe_pause(void);

/**
 * Signals to the frontend to resume processing new frames.
 **/
void cl_fe_unpause(void);

#endif
