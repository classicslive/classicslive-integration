#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define CL_SESSION_ID_LENGTH 32

typedef enum
{
  CL_OK = 0,

  CL_ERR_UNKNOWN,
  CL_ERR_USER_CONFIG,
  CL_ERR_CLIENT_RUNTIME,
  CL_ERR_CLIENT_COMPILE,
  CL_ERR_SERVER,

  CL_ERR_SIZE
} cl_error;

typedef struct
{
  char title[64];
  char description[256];
  char icon_url[64];
  unsigned flags;
  unsigned id;
  bool unlocked;
} cl_achievement_t;

typedef struct
{
  char title[64];
  char description[256];
  char details[256];
  char icon_url[64];
  unsigned id;
} cl_leaderboard_t;

typedef struct
{
  char title[64];
  unsigned value;
} cl_memnote_ex_value_t;

typedef struct
{
  char title[256];
  char description[2048];
  cl_memnote_ex_value_t values[64];
} cl_memnote_ex_t;

typedef union
{
  int64_t intval;
  double floatval;
  uint64_t uintval;
} cl_arg_t;

typedef enum
{
  CL_GAMEIDENTIFIER_INVALID = 0,

  CL_GAMEIDENTIFIER_FILE_HASH,
  CL_GAMEIDENTIFIER_PRODUCT_CODE,

  CL_GAMEIDENTIFIER_SIZE
} cl_game_identifer_type;

typedef struct
{
  cl_game_identifer_type type;

  const char *library;
  const char *filename;

  void *data;
  unsigned size;
  char checksum[64];

  char product[32];
  char version[32];
} cl_game_identifier_t;

typedef struct
{
  const char *data;
  unsigned error_code;
  const char *error_msg;
} cl_network_response_t;

typedef void (*cl_network_cb_t)(const cl_network_response_t, void*);
#define CL_NETWORK_CB(name) void name(const cl_network_response_t response, void *userdata)

typedef union cl_session_flags_t
{
  unsigned raw;
  struct
  {
    unsigned cheats : 1;
    unsigned states : 1;
    unsigned fast_forward : 1;
    unsigned pause : 1;
  } bits;
} cl_session_flags_t;

typedef enum
{
  CL_SESSION_NONE = 0,

  /**
   * The integration is currently posting to the 'login' endpoint.
   * The user does not have a session ID yet.
   */
  CL_SESSION_LOGGING_IN,

  /**
   * The integration has successfully posted to the 'login' endpoint.
   * The user is "online" and has a session ID but is not yet playing a game.
   */
  CL_SESSION_LOGGED_IN,

  /**
   * The integration is currently posting to the 'start' endpoint.
   * The user is "online" and has a session ID but is not yet playing a game.
   */
  CL_SESSION_STARTING,

  /**
   * The integration has successfully posted to the 'start' endpoint.
   * The user is "online," has a session ID, and is playing a game.
   */
  CL_SESSION_STARTED,

  CL_SESSION_SIZE
} cl_session_state;

typedef struct cl_session_t
{
  char checksum[64];
  char content_name[256];
  unsigned game_id;
  char game_title[256];
  char generic_post[2048];
  cl_session_flags_t flags;
  char id[CL_SESSION_ID_LENGTH];
  time_t last_status_update;

  cl_session_state state;

  cl_game_identifier_t identifier;

  cl_achievement_t *achievements;
  unsigned achievement_count;

  cl_leaderboard_t *leaderboards;
  unsigned leaderboard_count;
} cl_session_t;

struct cl_task_t;

typedef void (*CL_TASK_CB_T)(struct cl_task_t*);
#define CL_TASK_CB(name) void name(struct cl_task_t *task)

/**
 * A definition for a background task that the frontend will break off into a
 * separate thread.
 */
typedef struct cl_task_t
{
  /** The user-defined state that will be acted upon by the handler */
  void *state;

  /** The function that will be run in a new thread */
  CL_TASK_CB_T handler;

  /** The function that will be run after the handler has finished */
  CL_TASK_CB_T callback;

  /**
   * The reason that caused the handler to fail. Should be set if something
   * went wrong, otherwise NULL.
   */
  char *error;
} cl_task_t;

typedef struct
{
  char username[64];
  char password[64];
  char token[32];
  char language[8];
} cl_user_t;

/** A virtual address for the emulated system. */
typedef uintptr_t cl_addr_t;

#endif
