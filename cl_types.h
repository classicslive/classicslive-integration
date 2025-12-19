#ifndef CL_TYPES_H
#define CL_TYPES_H

#include "cl_config.h"

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

  CL_ERR_SERVER_NOT_FOUND,
  CL_ERR_SERVER_UNAVAILABLE,
  CL_ERR_SERVER_INTERNAL,
  CL_ERR_SERVER_UNEXPECTED_RESPONSE,

  CL_ERR_PARAMETER_INVALID,
  CL_ERR_PARAMETER_NULL,

  CL_ERR_SESSION_MISMATCH,

  CL_ERR_SIZE
} cl_error;

typedef enum
{
  CL_COMPARE_INVALID = 0,

  CL_COMPARE_EQUAL,
  CL_COMPARE_GREATER,
  CL_COMPARE_LESS,
  CL_COMPARE_NOT_EQUAL,
  CL_COMPARE_INCREASED,
  CL_COMPARE_DECREASED,
  CL_COMPARE_ABOVE,
  CL_COMPARE_BELOW,

  CL_COMPARE_SIZE
} cl_compare_type;

/**
 * A -1 value to represent invalid addresses in memory regions, as 0 for NULL
 * may be a valid address on some emulated systems.
 */
#define CL_ADDRESS_INVALID_INT 0xFFFFFFFFFFFFFFFF
#define CL_ADDRESS_INVALID ((void*)CL_ADDRESS_INVALID_INT)

/**
 * Maximum size, in bytes, to use when generating an MD5 hash of raw content
 * data.
 */
#define CL_CONTENT_SIZE_LIMIT 256 * 1024 * 1024

#define CL_GLOBALS_SIZE 3 * MAX_USERS

/**
 * Magic number representing the integration version. Server output may include
 * a minimum number so requests can be cancelled if the local integration is
 * not a recent enough build.
 */
#define CL_INTEGRATION_VERSION 1

#define CL_LOGGING true

/**
 * How often, in seconds, to ping back to the server to update current status.
 */
#define CL_PRESENCE_INTERVAL 60

/**
 * Radix used in output retrieved from the server.
 */
#define CL_RADIX 10

#define CL_SHOW_ERRORS true

/**
 * Format string to print the contents of a buffer representing an MD5 hash.
 */
#define CL_SNPRINTF_MD5 "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"

#define CL_TASK_MUTE true

/**
 * Used for unit tests to print a descriptive failure message.
 */
#define CL_TEST_FAIL(a) { printf("Test failed in %s on line %u.", \
  __FILE__, \
  __LINE__); \
  exit(a); };

typedef enum
{
  CL_ENDIAN_INVALID = 0,

  /* EFCDAB8967452301 */
  CL_ENDIAN_LITTLE,

  /* 0123456789ABCDEF */
  CL_ENDIAN_BIG,

  /* 67452301EFCDAB89 */
  CL_ENDIAN_WORD_FLIP_BL,

  /* 89ABCDEF01234567 */
  CL_ENDIAN_WORD_FLIP_LB,

#if CL_HOST_ENDIANNESS == _CL_ENDIANNESS_BIG
  CL_ENDIAN_NATIVE = _CL_ENDIANNESS_BIG,
#else
  CL_ENDIAN_NATIVE = _CL_ENDIANNESS_LITTLE,
#endif

  CL_ENDIAN_SIZE
} cl_endianness;

typedef enum
{
  CL_MSG_DEBUG = 0,

  CL_MSG_INFO,
  CL_MSG_WARN,
  CL_MSG_ERROR,

  CL_MSG_SIZE
} cl_log_level;

typedef enum
{
  CL_MEMTYPE_NOT_SET = 0,

  CL_MEMTYPE_INT64,
  CL_MEMTYPE_DOUBLE,

  CL_MEMTYPE_INT8,
  CL_MEMTYPE_UINT8,
  CL_MEMTYPE_INT16,
  CL_MEMTYPE_UINT16,
  CL_MEMTYPE_INT32,
  CL_MEMTYPE_UINT32,
  CL_MEMTYPE_FLOAT,

  CL_MEMTYPE_SIZE
} cl_value_type;

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
  unsigned value_count;
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
  /** The type of game identifier being used. */
  cl_game_identifer_type type;

  /**
   * The name of the emulator library or game being launched.
   * Required in all cases.
   */
  const char *library;

  /**
   * The filename of the game content.
   * Required in all cases.
   */
  char filename[256];

  /**
   * A pointer to game data to be checksummed.
   * Required when using CL_GAMEIDENTIFIER_FILE_HASH.
   */
  void *data;

  /**
   * The size, in bytes, of the game data to be checksummed.
   * Required when using CL_GAMEIDENTIFIER_FILE_HASH.
   */
  unsigned size;

  /**
   * The MD5 checksum of the game data.
   * Required when using CL_GAMEIDENTIFIER_FILE_HASH.
   */
  char checksum[64];

  /**
   * When using CL_GAMEIDENTIFIER_PRODUCT_CODE, the product code.
   * Required when using CL_GAMEIDENTIFIER_PRODUCT_CODE.
   * The exact format depends on the platform.
   */
  char product[32];

  /**
   * When using CL_GAMEIDENTIFIER_PRODUCT_CODE, the version string.
   * Required when using CL_GAMEIDENTIFIER_PRODUCT_CODE, optional otherwise.
   * The exact format depends on the platform.
   */
  char version[32];
} cl_game_identifier_t;

typedef struct
{
  const char *data;
  unsigned error_code;
  const char *error_msg;
} cl_network_response_t;

typedef void (*cl_network_cb_t)(const cl_network_response_t, void*);
#define CL_NETWORK_CB(name) \
  void name(const cl_network_response_t response, void *userdata)

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
#if CL_HOST_BITNESS == _CL_BITNESS_32
  #define CL_ADDRF "%08X"
  #define CL_SIZEF "%u"
#else
  #define CL_ADDRF "%016lX"
  #define CL_SIZEF "%lu"
#endif

typedef struct cl_counter_t
{
  union
  {
    int64_t i64;
    uint64_t raw;
  } intval;
  union
  {
    double fp;
    uint64_t raw;
  } floatval;
  cl_value_type type;
} cl_counter_t;

#define CL_POINTER_MAX_PASSES 8

/**
 * A bitfield to represent aspects of a mapped memory region of another
 * process. Primarily used when inspecting a memory region owned by
 * another process.
 */
typedef union cl_memory_region_flags
{
  struct
  {
    unsigned commit : 1;
    unsigned free : 1;
    unsigned reserve : 1;
    unsigned read : 1;
    unsigned write : 1;
    unsigned execute : 1;
    unsigned image : 1;
    unsigned mapped : 1;
    unsigned privated : 1;
  } bits;
  unsigned raw;
} cl_memory_region_flags;

typedef struct cl_memory_region_t
{
  /**
   * Flags that represent how this region can be utilized. Primarily used when
   * reading the memory of an external process.
   * @see cl_memory_region_flags
   */
  cl_memory_region_flags flags;

  /**
   * The byte ordering of this region.
   * @see cl_endianness
   */
  cl_endianness endianness;

  /**
   * A pointer to the location of this memory on the host system. When reading
   * memory from an external process, this will not be a directly accessible
   * pointer, but a location we use to request process memory from the
   * operating system.
   */
  void *base_host;

  /**
   * A pointer to the allocation base location this memory region is
   * contained within. Might be the same as base_host, or CL_ADDRESS_INVALID.
   * Should only be needed when reading external process memory, if ever.
   */
  void *base_alloc;

  /**
   * For emulators, the start of the virtual address for this region in the
   * emulated system, otherwise CL_ADDRESS_INVALID (we do not use NULL here
   * because 0 can be a valid guest memory location).
   */
  cl_addr_t base_guest;

  /** The size, in bytes, of this region. */
  cl_addr_t size;

  /**
   * The size, in bytes, of pointers within this region. Primarily used for
   * emulators that use a smaller pointer length than the host, like 2 or 4.
   */
  unsigned pointer_length;

  /**
   * A string that represents the purpose of this region. Primarily used for
   * virtually-mapped hardware devices on emulators.
   */
  char title[256];
} cl_memory_region_t;

/**
 * A "memory note" or "memnote" is a point in core memory that corresponds
 * with an observable in-game value. Instead of accessing specific addreses
 * every frame, we allocate memory notes and update their contents, then look 
 * off of them for script conditions.
 *
 * Here, we keep track of a memnote's value on the current and previous
 * frame, as well as the last unique value it had before it became
 * what it currently is. 
 */
typedef struct cl_memnote_t
{
  /* Random unique identifier for the memory note, between 0 and 9999 */
  unsigned key;

  /* The order of this memory note in the list */
  unsigned order;

  /**
   * The final virtual address this memory note points to, after pointer
   * dereferencing. This value is expected to change between frames, and is
   * only guaranteed to be valid after calling `cl_memory_update`.
   */
  cl_addr_t address;

  /**
   * The initial address or base pointer for this memory note, before pointer
   * dereferencing.
   */
  cl_addr_t address_initial;

  unsigned flags;

  /* The data type this memory note represents */
  cl_value_type type;

  /* Stored values */
  cl_counter_t current;
  cl_counter_t previous;
  cl_counter_t last_unique;

  /* For following pointers to get RAM values */
  unsigned pointer_offsets[CL_POINTER_MAX_PASSES];
  unsigned pointer_passes;

#if CL_HAVE_EDITOR
  /* Metadata for generated human-readable strings in Live Editor */
  cl_memnote_ex_t details;
  bool edited;
#endif
} cl_memnote_t;

/**
 * The global struct that contains information related to the content's memory,
 * including user-defined memory notes.
 */
typedef struct cl_memory_t
{
  cl_memnote_t *notes;
  unsigned note_count;

  cl_memory_region_t *regions;
  unsigned region_count;
} cl_memory_t;

#define CL_KB(a) ((cl_addr_t)(a) << 10)
#define CL_MB(a) ((cl_addr_t)(a) << 20)
#define CL_GB(a) ((cl_addr_t)(a) << 30)

#endif
