#ifndef CL_MAIN_H
#define CL_MAIN_H

#include "cl_types.h"

/**
 * Sends a request to the login endpoint and creates a new session. Should 
 * always be called before invoking `cl_start` or any other client operations
 * that require authentication, unless `cl_login_and_start` is used instead.
 *
 * @return `CL_OK` on success, or an appropriate error code on failure.
 */
cl_error cl_login(void);

/**
 * Sends a request to the login endpoint to identify the currently played game
 * and retrieve info about it. Requires a session established by `cl_login`.
 *
 * @param identifier The unique identifier of the game to start.
 * @return `CL_OK` on success, or an appropriate error code on failure.
 */
cl_error cl_start(cl_game_identifier_t identifier);

/**
 * Runs the identification code first, then performs login and start at the
 * same time. This should be used in cases where there is no reasonable
 * program state in which an idle session can be created, or if the game
 * identifier leaves scope quickly.
 *
 * @param identifier The unique identifier of the game to start.
 * @return `CL_OK` on success, or an appropriate error code on failure.
 */
cl_error cl_login_and_start(cl_game_identifier_t identifier);

/**
 * Executes the main runtime loop. Should be called once per frame.
 *
 * @return `CL_OK` on completion, or an appropriate error code on failure.
 */
cl_error cl_run(void);

/**
 * Cleans up memory and posts to the close endpoint if there is an active
 * session. Should be called once the client has finished running.
 * 
 * @return `CL_OK` on completion.
 */
cl_error cl_free(void);

extern cl_session_t session;

#endif
