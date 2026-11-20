#include "cl_abi.h"
#include "cl_counter.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_script.h"
#include "cl_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CL_TEST_DATA_SIZE 128
#define CL_TEST_REGION_COUNT 4

#ifndef CL_TEST_REGION_SIZE
#define CL_TEST_REGION_SIZE CL_MB(16)
#endif

typedef struct
{
  cl_memory_region_t regions[CL_TEST_REGION_COUNT];
  unsigned char data[CL_TEST_DATA_SIZE];
  cl_game_identifier_t identifier;
} cl_test_system_t;

static cl_test_system_t cl_test_system;
static unsigned cl_test_thread_depth = 0;
static char cl_test_msg[256];

/* Achievement submissions the fake server has received, and the last one */
static unsigned cl_test_achievement_posts = 0;
static char cl_test_achievement_data[256];
static unsigned cl_test_leaderboard_posts = 0;
static char cl_test_leaderboard_data[512];

static cl_error cl_test_display_message(unsigned level, const char *msg)
{
  const char *level_str;
  unsigned i;

  switch (level)
  {
    case CL_MSG_DEBUG: level_str = "[DEBUG]"; break;
    case CL_MSG_INFO: level_str = "[INFO ]"; break;
    case CL_MSG_WARN: level_str = "[WARN ]"; break;
    case CL_MSG_ERROR: level_str = "[ERROR]"; break;
    default: exit(CL_ERR_PARAMETER_INVALID);
  };
  for (i = 0; i < cl_test_thread_depth; i++)
  {
    if (i == cl_test_thread_depth - 1)
      printf(" - ");
    else
      printf("   ");
  }
  printf("classicslive-integration %s: %s\n", level_str, msg);

  return CL_OK;
}

static cl_error cl_test_install_memory_regions(cl_memory_region_t **regions,
  unsigned *region_count)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_install_memory_regions - regions:%p region_count:%p",
    (void*)regions, (void*)region_count);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  if (!regions || !region_count)
    exit(CL_ERR_PARAMETER_NULL);
  else
  {
    *regions = (cl_memory_region_t*)malloc(
      sizeof(cl_memory_region_t) * CL_TEST_REGION_COUNT);
    if (!*regions)
      exit(CL_ERR_CLIENT_RUNTIME);
    memcpy(*regions, cl_test_system.regions,
           sizeof(cl_memory_region_t) * CL_TEST_REGION_COUNT);
    *region_count = CL_TEST_REGION_COUNT;

    return CL_OK;
  }
}

static cl_error cl_test_library_name(const char **name)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_library_name - name:%p", (void*)name);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  if (!name)
    exit(CL_ERR_PARAMETER_NULL);
  else
  {
    *name = "classicslive-integration-test";
    return CL_OK;
  }
}

static cl_error cl_test_network_post(const char *url, char *data,
  cl_network_cb_t callback, void *userdata)
{
  cl_network_response_t response;

  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_network_post - url:%s data:%s userdata:%p",
    url, data ? data : "(null)", (void*)userdata);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  response.error_code = 0;
  response.error_msg = NULL;
  /**
   * This acts as a fake Classics Live server that can service certain
   * requests to mock a simple session. As a result of the simplicity,
   * requests may work here that would not work on a real server.
   */
  if (strstr(url, CL_CLINT_URL CL_END_CLINT_LOGIN))
  {
    response.data =
      "{"
      "\"success\":true,"
      "\"session_id\":\"#000000000000000000000000000000\""
      "}\n";
  }
  else if (strstr(url, CL_CLINT_URL CL_END_CLINT_START))
  {
    response.data =
      "{"
        "\"success\":true,"
        "\"game_id\":1,"
        "\"title\":\"Test Game\","
        "\"memory_notes\":"
        "["
          "{\"id\":1,\"type\":8,\"offsets\":\"1 4\",\"address\":268435456}"
        "],"
        "\"achievements\":"
        "["
          "{\"id\":1,\"title\":\"High Five\",\"description\":\""
            "Increment a value to equal 5.\"},"
          "{\"id\":2,\"title\":\"Not Happening\",\"description\":\""
            "You will never achieve this.\"}"
        "],"
        "\"leaderboards\":"
        "["
          "{\"id\":1,\"title\":\"High Scores\",\"description\":\""
            "Top scores for the game.\"}"
        "],"
        "\"script\":\"1 2 0 15 5 1 1 0 5 1 1 18 2 0 1\""
      "}\n";
  }
  else if (strstr(url, CL_CLINT_URL CL_END_CLINT_ACHIEVEMENT))
  {
    cl_test_achievement_posts++;
    snprintf(cl_test_achievement_data, sizeof(cl_test_achievement_data),
             "%s", data ? data : "");
    response.data =
      "{"
        "\"success\":true"
      "}\n";
  }
  else if (strstr(url, CL_CLINT_URL CL_END_CLINT_LEADERBOARD))
  {
    cl_test_leaderboard_posts++;
    snprintf(cl_test_leaderboard_data, sizeof(cl_test_leaderboard_data),
             "%s", data ? data : "");
    response.data =
      "{"
        "\"success\":true,"
        "\"leaderboard_id\":1,"
        "\"score\":1234,"
        "\"display\":\"1,234\","
        "\"personal_best\":true"
      "}\n";
  }
  else if (strstr(url, CL_CLINT_URL CL_END_CLINT_CLOSE))
  {
    response.data =
      "{"
        "\"success\":true"
      "}\n";
  }
  else
  {
    response.data = NULL;
    response.error_code = 1;
    response.error_msg = "Unknown endpoint";
  }
  if (callback)
    callback(response, userdata);
  
  return CL_OK;
}

static cl_error cl_test_thread(cl_task_t *task)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_thread - task:%p", (void*)task);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  if (!task || !task->handler)
    exit(CL_ERR_PARAMETER_NULL);

  /* Run the handler directly */
  cl_test_thread_depth++;
  printf(" - Running task handler...\n");
  task->error = NULL;
  task->handler(task);
  printf(" - Task handler completed.\n");

  /* Run the callback directly */
  printf(" - Running task callback...\n");
  if (task->callback)
    task->callback(task->state);
  printf(" - Task callback completed.\n");
  cl_test_thread_depth--;

  if (task->state)
    free(task->state);
  free(task);

  return CL_OK;
}

static cl_error cl_test_set_pause(unsigned mode)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_set_pause - mode:%u", mode);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  return CL_OK;
}

static cl_error cl_test_user_data(cl_user_t *user, unsigned index)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_user_data - user:%p index:%u", (void*)user, index);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  if (!user)
    return CL_ERR_PARAMETER_NULL;
  if (index != 0)
    return CL_ERR_PARAMETER_INVALID;
  snprintf(user->username, sizeof(user->username), "clint");
  snprintf(user->password, sizeof(user->password), "coffeecoffeefrog123");
  user->token[0] = '\0';
  snprintf(user->language, sizeof(user->language), "en_US");

  return CL_OK;
}

static cl_error cl_test_external_read_buffer(void *dest, cl_addr_t address,
  unsigned size, unsigned *read)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_read - dest:%p address:0x%08x size:%u read:%p",
    dest, (unsigned)address, size, (void*)read);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  return cl_read_memory_buffer_internal(dest, NULL, address, size);
}

static cl_error cl_test_external_read_value(void *dest, cl_addr_t address,
  cl_value_type type)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_read_value - dest:%p address:0x%08x type:%u",
    dest, (unsigned)address, type);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  return cl_read_memory_value_internal(dest, NULL, address, type);
}

static cl_error cl_test_external_write_buffer(const void *src, cl_addr_t address,
  unsigned size, unsigned *written)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_write - src:%p address:0x%08x size:%u written:%p",
    src, (unsigned)address, size, (void*)written);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  return cl_write_memory_buffer_internal(src, NULL, address, size);
}

static cl_error cl_test_external_write_value(const void *src, cl_addr_t address,
  cl_value_type type)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_write_value - src:%p address:0x%08x type:%u",
    src, (unsigned)address, type);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

  return cl_write_memory_value_internal(src, NULL, address, type);
}

static const cl_abi_t cl_test_abi =
{
  CL_ABI_VERSION,
  {
    {
      cl_test_display_message,
      cl_test_install_memory_regions,
      cl_test_library_name,
      cl_test_network_post,
      cl_test_set_pause,
      cl_test_thread,
      cl_test_user_data
    },
    {
      cl_test_external_read_buffer,
      cl_test_external_read_value,
      cl_test_external_write_buffer,
      cl_test_external_write_value
    }
  }
};

static cl_error cl_test_console_init(void)
{
  unsigned i, j;

  /* Setup fake memory regions */
  cl_test_system.regions[0].base_guest = 0x10000000;
  cl_test_system.regions[0].size = CL_TEST_REGION_SIZE;
  cl_test_system.regions[0].base_host = malloc(CL_TEST_REGION_SIZE);
  cl_test_system.regions[0].endianness = CL_ENDIAN_NATIVE;
  cl_test_system.regions[0].pointer_length = 4;
  snprintf(cl_test_system.regions[0].title,
           sizeof(cl_test_system.regions[0].title),
           "Test Region 0");

  cl_test_system.regions[1].base_guest = 0x20000000;
  cl_test_system.regions[1].size = CL_TEST_REGION_SIZE;
  cl_test_system.regions[1].base_host = malloc(CL_TEST_REGION_SIZE);
  cl_test_system.regions[1].endianness = CL_ENDIAN_NATIVE;
  cl_test_system.regions[1].pointer_length = 4;
  snprintf(cl_test_system.regions[1].title,
           sizeof(cl_test_system.regions[1].title),
           "Test Region 1");

  cl_test_system.regions[2].base_guest = 0x30000000;
  cl_test_system.regions[2].size = CL_TEST_REGION_SIZE;
  cl_test_system.regions[2].base_host = malloc(CL_TEST_REGION_SIZE);
  cl_test_system.regions[2].endianness = CL_ENDIAN_LITTLE;
  cl_test_system.regions[2].pointer_length = 4;
  snprintf(cl_test_system.regions[2].title,
           sizeof(cl_test_system.regions[2].title),
           "Test Region 2 (little-endian)");

  cl_test_system.regions[3].base_guest = 0x40000000;
  cl_test_system.regions[3].size = CL_TEST_REGION_SIZE;
  cl_test_system.regions[3].base_host = malloc(CL_TEST_REGION_SIZE);
  cl_test_system.regions[3].endianness = CL_ENDIAN_BIG;
  cl_test_system.regions[3].pointer_length = 4;
  snprintf(cl_test_system.regions[3].title,
           sizeof(cl_test_system.regions[3].title),
           "Test Region 3 (big-endian)");

  /* Fill with nonsense */
  for (i = 0; i < CL_TEST_REGION_COUNT; i++)
    for (j = 0; j < cl_test_system.regions[i].size; j++)
      ((unsigned char*)cl_test_system.regions[i].base_host)[j] = ((j & 0x0C) >> 2);

  /* Setup fake game to be identified */
  cl_test_system.identifier.type = CL_GAMEIDENTIFIER_FILE_HASH;
  cl_test_system.identifier.data = cl_test_system.data;
  cl_test_system.identifier.size = CL_TEST_DATA_SIZE;
  snprintf(cl_test_system.identifier.filename,
           sizeof(cl_test_system.identifier.filename),
           "classicslive-integration-test.rom");
  for (i = 0; i < CL_TEST_DATA_SIZE; i++)
    cl_test_system.data[i] = (i & 0xFF);

  return CL_OK;
}

/* Script logic test pieces: conditions on counters 0-2, results in 14-15 */
#define CL_TEST_IF_A "0 15 5 5 0 0 1 1 "
#define CL_TEST_IF_B "0 15 5 5 1 0 1 1 "
#define CL_TEST_IF_C "0 15 5 5 2 0 1 1 "
#define CL_TEST_OR "0 24 0 "
#define CL_TEST_THEN "1 5 3 15 0 1 "
#define CL_TEST_ELSE "0 23 0 1 5 3 15 0 2 "

/* If A { if B { 15 = 1 } else { 15 = 2 } } else { 15 = 3 }, then 14 = 7 */
#define CL_TEST_NESTED \
  "0 15 5 5 0 0 1 1 1 15 5 5 1 0 1 1 2 5 3 15 0 1 1 23 0 2 5 3 15 0 2 " \
  "0 23 0 1 5 3 15 0 3 0 5 3 14 0 7 "

/* Expected result when the script is rejected at load time */
#define CL_TEST_REJECTED -1

typedef struct
{
  const char *logic;
  int a, b, c;

  /* counter 15 * 100 + counter 14, or CL_TEST_REJECTED */
  cl_int64 expected;
} cl_test_script_case_t;

static const cl_test_script_case_t cl_test_script_cases[] =
{
  /* Plain AND chains behave as before */
  { CL_TEST_IF_A CL_TEST_IF_B CL_TEST_THEN, 1, 1, 0, 100 },
  { CL_TEST_IF_A CL_TEST_IF_B CL_TEST_THEN, 1, 0, 0, 0 },

  /* A OR B */
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_THEN CL_TEST_ELSE, 0, 0, 0, 200 },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_THEN CL_TEST_ELSE, 1, 0, 0, 100 },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_THEN CL_TEST_ELSE, 0, 1, 0, 100 },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_THEN CL_TEST_ELSE, 1, 1, 0, 100 },

  /* (A OR B) AND C */
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 1, 0, 0, 200 },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 0, 1, 1, 100 },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_IF_B CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 0, 0, 1, 200 },

  /* A AND (B OR C) */
  { CL_TEST_IF_A CL_TEST_IF_B CL_TEST_OR CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 1, 0, 1, 100 },
  { CL_TEST_IF_A CL_TEST_IF_B CL_TEST_OR CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 0, 1, 1, 200 },
  { CL_TEST_IF_A CL_TEST_IF_B CL_TEST_OR CL_TEST_IF_C CL_TEST_THEN CL_TEST_ELSE, 1, 0, 0, 200 },

  /* Nested if/else under an if/else, then an action that always runs */
  { CL_TEST_NESTED, 1, 1, 0, 107 },
  { CL_TEST_NESTED, 1, 0, 0, 207 },
  { CL_TEST_NESTED, 0, 1, 0, 307 },

  /* Misplaced OR and ELSE are rejected at load time */
  { CL_TEST_OR CL_TEST_IF_A CL_TEST_THEN, 0, 0, 0, CL_TEST_REJECTED },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_THEN, 0, 0, 0, CL_TEST_REJECTED },
  { CL_TEST_IF_A CL_TEST_OR CL_TEST_OR CL_TEST_IF_B CL_TEST_THEN, 0, 0, 0, CL_TEST_REJECTED },
  { "0 15 5 5 0 0 1 1 1 24 0 0 15 5 5 1 0 1 1 " CL_TEST_THEN, 0, 0, 0, CL_TEST_REJECTED },
  { CL_TEST_ELSE, 0, 0, 0, CL_TEST_REJECTED },
  { CL_TEST_IF_A CL_TEST_THEN CL_TEST_ELSE CL_TEST_ELSE, 0, 0, 0, CL_TEST_REJECTED }
};

/**
 * Counts the actions in serialized script actions.
 * @param text Actions as "<indent> <type> <argc> <args...>" repeated.
 * @return The number of actions.
 */
static unsigned cl_test_count_actions(const char *text)
{
  char *end;
  unsigned count = 0;
  long argc;

  while (*text)
  {
    strtol(text, &end, 10);
    if (end == text)
      break;
    strtol(end, &end, 10);
    argc = strtol(end, &end, 10);
    while (argc-- > 0)
      strtol(end, &end, 10);
    text = end;
    count++;
    while (*text == ' ')
      text++;
  }

  return count;
}

/**
 * Runs each script logic case for one frame through the real interpreter.
 * @return CL_OK if every case gave its expected result.
 */
static cl_error cl_test_script_logic(void)
{
  const cl_test_script_case_t *test;
  char text[512];
  const char *pos;
  cl_int64 result;
  unsigned i;

  for (i = 0; i < sizeof(cl_test_script_cases) / sizeof(cl_test_script_cases[0]); i++)
  {
    test = &cl_test_script_cases[i];

    /* Three actions set the inputs before the case's own logic */
    snprintf(text, sizeof(text),
      "1 %u 0 5 3 0 0 %d 0 5 3 1 0 %d 0 5 3 2 0 %d %s",
      3 + cl_test_count_actions(test->logic), test->a, test->b, test->c,
      test->logic);
    pos = text;
    if (cl_script_init(&pos) != CL_OK)
      result = CL_TEST_REJECTED;
    else
    {
      cl_script_update();
      result = script.pages[0].counters[15].intval.i64 * 100 +
               script.pages[0].counters[14].intval.i64;
    }
    cl_script_free();
    if (result != test->expected)
    {
      printf("Script logic case %u failed: expected %ld, got %ld\n", i,
             (long)test->expected, (long)result);
      return CL_ERR_CLIENT_RUNTIME;
    }
  }

  return CL_OK;
}

/**
 * Loads, runs and frees a two-page script, then frees a script that failed
 * partway through loading its second page. Run under a memory checker to
 * catch bad frees.
 * @return CL_OK if both pages ran and nothing crashed.
 */
static cl_error cl_test_script_pages(void)
{
  const char *two_pages = "2 1 0 5 3 15 0 1 1 0 5 3 15 0 2";
  const char *truncated = "2 1 0 5 3 15 0 1 2 0 5 3 15 0 2";
  const char *pos;
  cl_int64 first, second;

  pos = two_pages;
  if (cl_script_init(&pos) != CL_OK)
    return CL_ERR_CLIENT_RUNTIME;
  cl_script_update();
  first = script.pages[0].counters[15].intval.i64;
  second = script.pages[1].counters[15].intval.i64;
  cl_script_free();
  if (first != 1 || second != 2)
  {
    printf("Script page test failed: pages gave %ld and %ld\n",
           (long)first, (long)second);
    return CL_ERR_CLIENT_RUNTIME;
  }

  pos = truncated;
  if (cl_script_init(&pos) == CL_OK)
  {
    printf("Script page test failed: truncated script loaded\n");
    return CL_ERR_CLIENT_RUNTIME;
  }
  cl_script_free();

  return CL_OK;
}

/* One frame of the dynamic unlock test: counter 0's value and expected posts */
typedef struct
{
  cl_int64 id;
  unsigned posts;
} cl_test_unlock_frame_t;

static const cl_test_unlock_frame_t cl_test_unlock_frames[] =
{
  { 1, 1 }, /* Unlocks 1 */
  { 1, 1 }, /* Already submitted */
  { 2, 2 }, /* The same action unlocks 2 */
  { 3, 2 }, /* Not an achievement of this game */
  { 0, 2 },
  { 2, 2 }  /* Already submitted */
};

/**
 * Runs a single "unlock achievement[counter 0]" action while counter 0
 * changes, checking each achievement is submitted once, then that reloading
 * the script allows submitting again. Needs a session with achievements 1, 2.
 * @return CL_OK if every frame posted what it should.
 */
static cl_error cl_test_dynamic_unlock(void)
{
  const char *text = "1 1 0 18 2 5 0";
  const char *pos;
  unsigned base, i;
  unsigned count = sizeof(cl_test_unlock_frames) / sizeof(cl_test_unlock_frames[0]);

  cl_script_free();
  pos = text;
  if (cl_script_init(&pos) != CL_OK)
    return CL_ERR_CLIENT_RUNTIME;
  base = cl_test_achievement_posts;

  /* The final pass reloads the script and expects achievement 1 again */
  for (i = 0; i <= count; i++)
  {
    if (i == count)
    {
      cl_script_free();
      pos = text;
      cl_script_init(&pos);
    }
    cl_ctr_store_int(&script.pages[0].counters[0],
                     i < count ? cl_test_unlock_frames[i].id : 1);
    cl_script_update();
    if (cl_test_achievement_posts - base !=
        (i < count ? cl_test_unlock_frames[i].posts : 3))
    {
      printf("Dynamic unlock frame %u failed: %u posts, last \"%s\"\n", i,
             cl_test_achievement_posts - base, cl_test_achievement_data);
      return CL_ERR_CLIENT_RUNTIME;
    }
  }
  if (strcmp(cl_test_achievement_data, "achievement_id=1") &&
      strncmp(cl_test_achievement_data, "achievement_id=1&", 17))
  {
    printf("Dynamic unlock posted the wrong data: \"%s\"\n",
           cl_test_achievement_data);
    return CL_ERR_CLIENT_RUNTIME;
  }

  return CL_OK;
}

/**
 * A leaderboard action under a condition submits once each time the
 * condition becomes true, not on every frame it stays true, and sends the
 * page's counters (signed) with it. Unknown leaderboard IDs are ignored.
 * Counter 0 holds the leaderboard ID, counter 1 the condition, counter 2 a
 * negative value. The script is "if c1 == 1: post leaderboard[c0]".
 */
typedef struct
{
  unsigned id;
  unsigned condition;
  unsigned posts;
} cl_test_ldb_frame_t;

static const cl_test_ldb_frame_t cl_test_ldb_frames[] =
{
  { 1, 1, 1 }, /* condition becomes true: submit */
  { 1, 1, 1 }, /* still true: don't resubmit */
  { 1, 0, 1 }, /* false */
  { 1, 1, 2 }, /* true again: submit */
  { 1, 0, 2 },
  { 9, 1, 2 }, /* this game has no leaderboard 9 */
};

static cl_error cl_test_leaderboard(void)
{
  const char *text = "1 2 0 15 5 5 1 0 1 1 1 19 2 5 0";
  const char *pos = text;
  const char *expected = "leaderboard_id=1&c0=1&c1=1&c2=-7&c3=0";
  unsigned count = sizeof(cl_test_ldb_frames) / sizeof(cl_test_ldb_frames[0]);
  unsigned base, i;

  cl_script_free();
  if (cl_script_init(&pos) != CL_OK)
    return CL_ERR_CLIENT_RUNTIME;
  base = cl_test_leaderboard_posts;
  cl_ctr_store_int(&script.pages[0].counters[2], -7);

  for (i = 0; i < count; i++)
  {
    cl_ctr_store_int(&script.pages[0].counters[0], cl_test_ldb_frames[i].id);
    cl_ctr_store_int(&script.pages[0].counters[1], cl_test_ldb_frames[i].condition);
    cl_script_update();
    if (cl_test_leaderboard_posts - base != cl_test_ldb_frames[i].posts)
    {
      printf("Leaderboard frame %u failed: %u posts, last \"%s\"\n", i,
             cl_test_leaderboard_posts - base, cl_test_leaderboard_data);
      return CL_ERR_CLIENT_RUNTIME;
    }
  }
  if (strncmp(cl_test_leaderboard_data, expected, strlen(expected)))
  {
    printf("Leaderboard posted the wrong data: \"%s\"\n", cl_test_leaderboard_data);
    return CL_ERR_CLIENT_RUNTIME;
  }

  return CL_OK;
}

static cl_error cl_test_console_free(void)
{
  unsigned i;

  for (i = 0; i < CL_TEST_REGION_COUNT; i++)
  {
    free(cl_test_system.regions[i].base_host);
    cl_test_system.regions[i].base_host = NULL;
  }

  return CL_OK;
}

cl_error cl_test(void)
{
  cl_search_t search;
  clock_t start, end;
  double cpu_time_used;
  int error;
  unsigned int word;
  unsigned char byte;
  unsigned i;

  printf("Starting classicslive-integration tests for target:\n"
    "Platform: %s\nBitness: %s\nEndianness: %s\n\n",
    cl_string_platform(CL_HOST_PLATFORM),
    cl_string_bitness(CL_HOST_BITNESS),
    cl_string_endianness(CL_HOST_ENDIANNESS));
  
  printf("============================================================\n");
  printf("Performing sizeof int tests...\n");
  if (sizeof(cl_int64) != 8)
  {
    printf("Unexpected sizeof(cl_int64)! Tests cannot continue.\n");
    return CL_ERR_CLIENT_COMPILE;
  }
  else
    printf("sizeof(cl_int64) test passed!\n");
  if (sizeof(cl_uint64) != 8)
  {
    printf("Unexpected sizeof(cl_uint64)! Tests cannot continue.\n");
    return CL_ERR_CLIENT_COMPILE;
  }
  else
    printf("sizeof(cl_uint64) test passed!\n");

  printf("============================================================\n");
  printf("Performing basic counter function tests...\n");
  error = cl_ctr_tests();
  if (!error)
    printf("Counter tests passed!\n");
  else
  {
    printf("Counter test %d failed!\n", error);
    return error;
  }

  printf("============================================================\n");
  printf("Registering test ABI...\n");
  error = cl_abi_register(&cl_test_abi);
  if (error != CL_OK)
    return error;

  printf("============================================================\n");
  printf("Initializing test console...\n");
  error = cl_test_console_init();
  if (error != CL_OK)
    return error;

  printf("============================================================\n");
  printf("Performing script logic tests (OR, ELSE)...\n");
  error = cl_test_script_logic();
  if (error == CL_OK)
    error = cl_test_script_pages();
  if (error != CL_OK)
    return error;
  printf("Script logic tests passed!\n");

  printf("============================================================\n");
  printf("Performing login flow tests...\n");
  error = cl_login_and_start(cl_test_system.identifier);
  if (error != CL_OK)
    return error;

  for (i = 0; i < session.achievement_count; i++)
    printf("Achievement %u: %s - %s\n",
      session.achievements[i].id,
      session.achievements[i].title,
      session.achievements[i].description);
  for (i = 0; i < session.leaderboard_count; i++)
    printf("Leaderboard %u: %s - %s\n",
      session.leaderboards[i].id,
      session.leaderboards[i].title,
      session.leaderboards[i].description);

  printf("============================================================\n");
  printf("Performing virtual memory tests...\n");
  word = 0x0D15EA5E;
  cl_write_memory_value(&word, NULL, 0x30000000, CL_MEMTYPE_UINT32);
  cl_read_memory_value(&byte, NULL, 0x30000000, CL_MEMTYPE_UINT8);
  if (byte != 0x5E)
  {
    printf("Little-endian virtual memory read/write test failed (got 0x%02X)!\n", byte);
    return CL_ERR_CLIENT_RUNTIME;
  }
  else
    printf("Little-endian virtual memory read/write test passed (got 0x%02X)!\n", byte);
  cl_write_memory_value(&word, NULL, 0x40000000, CL_MEMTYPE_UINT32);
  cl_read_memory_value(&byte, NULL, 0x40000000, CL_MEMTYPE_UINT8);
  if (byte != 0x0D)
  {
    printf("Big-endian virtual memory read/write test failed (got 0x%02X)!\n", byte);
    return CL_ERR_CLIENT_RUNTIME;
  }
  else
    printf("Big-endian virtual memory read/write test passed (got 0x%02X)!\n", byte);

  printf("============================================================\n");
  printf("Initializing memory search...");
  cl_search_init(&search);
  printf("change cmp...");
  cl_search_change_compare_type(&search, CL_COMPARE_EQUAL);
  printf("change val...");
  cl_search_change_value_type(&search, CL_MEMTYPE_UINT32);
  printf("done.\n");
  
  printf("Compare to 0...");
  word = 0;
  memset(cl_test_system.regions[1].base_host, word, 16);
  cl_search_change_target(&search, &word);
  start = clock();
  cl_search_step(&search);
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf(CL_SIZEF " matches found. %u pages. " CL_SIZEF " memory usage. Time: %.6f s\n",
    search.total_matches, search.total_page_count, search.memory_usage, cpu_time_used);

  printf("Compare to 1...");
  word = 1;
  for (i = 0; i < cl_test_system.regions[1].size; i += 4)
    ((unsigned*)cl_test_system.regions[1].base_host)[i / 4] = word;
  for (i = 0; i < cl_test_system.regions[2].size; i += 4)
    ((unsigned*)cl_test_system.regions[2].base_host)[i / 4] = word;
  cl_search_change_target(&search, &word);
  start = clock();
  cl_search_step(&search);
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf(CL_SIZEF " matches found. %u pages. " CL_SIZEF " memory usage. Time: %.6f s\n",
    search.total_matches, search.total_page_count, search.memory_usage, cpu_time_used);
  
  printf("Compare to 2...");
  word = 2;
  ((unsigned*)cl_test_system.regions[1].base_host)[0] = word;
  cl_search_change_target(&search, &word);
  start = clock();
  cl_search_step(&search);
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf(CL_SIZEF " matches found. %u pages. " CL_SIZEF " memory usage. Time: %.6f s\n",
    search.total_matches, search.total_page_count, search.memory_usage, cpu_time_used);

  printf("Compare to 3...");
  word = 3;
  for (i = 0; i < 16; i += 4)
    ((unsigned*)cl_test_system.regions[1].base_host)[i / 4] = word;
  cl_search_change_target(&search, &word);
  start = clock();
  cl_search_step(&search);
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf(CL_SIZEF " matches found. %u pages. " CL_SIZEF " memory usage. Time: %.6f s\n",
    search.total_matches, search.total_page_count, search.memory_usage, cpu_time_used);

  if (search.total_matches != 1)
  {
    printf("Memory search test failed!\n");
    return CL_ERR_CLIENT_RUNTIME;
  }
  else
    printf("Memory search test passed!\n");
  printf("Freeing search...\n");
  cl_search_free(&search);

  printf("============================================================\n");
  printf("Running simulated frames...\n");
  printf("Achievement should unlock between 4 and 5...\n");
  word = 0x20000000;
  cl_write_memory_value(&word, NULL, 0x10000000, CL_MEMTYPE_UINT32);
  for (i = 0; i < 10; i++)
  {
    cl_write_memory_value(&i, NULL, 0x20000004, CL_MEMTYPE_UINT32);
    cl_run();
    cl_read_memory_value(&word, NULL, 0x20000004, CL_MEMTYPE_UINT32);
    printf(" - Frame %u completed, memory at 0x20000004 = 0x%08x\n", i, word);
  }

  printf("============================================================\n");
  printf("Performing dynamic achievement ID tests...\n");
  error = cl_test_dynamic_unlock();
  if (error != CL_OK)
    return error;
  printf("Dynamic achievement ID tests passed!\n");

  printf("============================================================\n");
  printf("Performing leaderboard tests...\n");
  error = cl_test_leaderboard();
  if (error != CL_OK)
    return error;
  printf("Leaderboard tests passed!\n");

  printf("============================================================\n");
  printf("Freeing test session...\n");
  error = cl_free();
  if (error != CL_OK)
    return error;
  printf("Freeing test console...\n");
  error = cl_test_console_free();
  if (error != CL_OK)
    return error;

  printf("\nd(. _ . )b All tests completed successfully!\n");

  return CL_OK;
}
