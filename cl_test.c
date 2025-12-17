#include "cl_abi.h"
#include "cl_counter.h"
#include "cl_main.h"
#include "cl_memory.h"
#include "cl_network.h"
#include "cl_search_new.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CL_TEST_DATA_SIZE 128
#define CL_TEST_REGION_COUNT 4
typedef struct
{
  cl_memory_region_t regions[CL_TEST_REGION_COUNT];
  unsigned char data[CL_TEST_DATA_SIZE];
  cl_game_identifier_t identifier;
} cl_test_system_t;

static cl_test_system_t cl_test_system;
static unsigned cl_test_thread_depth = 0;
static char cl_test_msg[256];

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
      "\"session_id\":\"#0000000000000000000000000000000\""
      "}";
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
          "{\"achievement_id\":1,\"name\":\"High Five\",\"description\":\""
            "Increment a value to equal 5.\"}"
        "],"
        "\"leaderboards\":"
        "["
          "{\"leaderboard_id\":1,\"name\":\"High Scores\",\"description\":\""
            "Top scores for the game.\"}"
        "],"
        "\"script\":\"1 2 0 15 5 1 1 0 5 1 1 18 2 0 1\""
      "}";
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
  snprintf(user->token, sizeof(user->token), "");
  snprintf(user->language, sizeof(user->language), "en_US");

  return CL_OK;
}

static cl_error cl_test_external_read(void *dest, cl_addr_t address,
  unsigned size, unsigned *read)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_read - dest:%p address:0x%08x size:%u read:%p",
    dest, (unsigned)address, size, (void*)read);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

#if CL_EXTERNAL_MEMORY
  if (!dest || !read)
    return CL_ERR_PARAMETER_NULL;

  /* Find the region that contains this address */
  for (unsigned i = 0; i < CL_TEST_REGION_COUNT; i++)
  {
    cl_memory_region_t *region = &cl_test_system.regions[i];

    if (address >= region->base_guest &&
        address + size <= region->base_guest + region->size)
    {
      /* Found the region, perform the read */
      cl_addr_t offset = address - region->base_guest;

      memcpy(dest,
             (unsigned char*)region->base_host + offset,
             size);
      *read = size;
      return CL_OK;
    }
  }

  /* Address not found */
  *read = 0;
  return CL_ERR_PARAMETER_INVALID;
#else
  CL_UNUSED(dest);
  CL_UNUSED(address);
  CL_UNUSED(size);
  CL_UNUSED(read);
  return CL_ERR_CLIENT_RUNTIME;
#endif
}

static cl_error cl_test_external_write(const void *src, cl_addr_t address,
  unsigned size, unsigned *written)
{
  snprintf(cl_test_msg, sizeof(cl_test_msg),
    "cl_abi_external_write - src:%p address:0x%08x size:%u written:%p",
    src, (unsigned)address, size, (void*)written);
  cl_test_display_message(CL_MSG_DEBUG, cl_test_msg);

#if CL_EXTERNAL_MEMORY
  if (!src || !written)
    return CL_ERR_PARAMETER_NULL;
  /* Find the region that contains this address */
  for (unsigned i = 0; i < CL_TEST_REGION_COUNT; i++)
  {
    cl_memory_region_t *region = &cl_test_system.regions[i];

    if (address >= region->base_guest &&
        address + size <= region->base_guest + region->size)
    {
      /* Found the region, perform the write */
      cl_addr_t offset = address - region->base_guest;

      memcpy((unsigned char*)region->base_host + offset,
             src,
             size);
      *written = size;
      return CL_OK;
    }
  }

  /* Address not found */
  *written = 0;
  return CL_ERR_PARAMETER_INVALID;
#else
  CL_UNUSED(src);
  CL_UNUSED(address);
  CL_UNUSED(size);
  CL_UNUSED(written);
  return CL_ERR_CLIENT_RUNTIME;
#endif
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
      cl_test_external_read,
      cl_test_external_write
    }
  }
};

static cl_error cl_test_console_init(void)
{
  static const unsigned region_size = CL_MB(16);
  unsigned i, j;

  /* Setup fake memory regions */
  cl_test_system.regions[0].base_guest = 0x10000000;
  cl_test_system.regions[0].size = region_size;
  cl_test_system.regions[0].base_host = malloc(region_size);
  cl_test_system.regions[0].endianness = CL_ENDIAN_NATIVE;
  cl_test_system.regions[0].pointer_length = 4;
  snprintf(cl_test_system.regions[0].title,
           sizeof(cl_test_system.regions[0].title),
           "Test Region 0");

  cl_test_system.regions[1].base_guest = 0x20000000;
  cl_test_system.regions[1].size = region_size;
  cl_test_system.regions[1].base_host = malloc(region_size);
  cl_test_system.regions[1].endianness = CL_ENDIAN_NATIVE;
  cl_test_system.regions[1].pointer_length = 4;
  snprintf(cl_test_system.regions[1].title,
           sizeof(cl_test_system.regions[1].title),
           "Test Region 1");

  cl_test_system.regions[2].base_guest = 0x30000000;
  cl_test_system.regions[2].size = region_size;
  cl_test_system.regions[2].base_host = malloc(region_size);
  cl_test_system.regions[2].endianness = CL_ENDIAN_LITTLE;
  cl_test_system.regions[2].pointer_length = 4;
  snprintf(cl_test_system.regions[2].title,
           sizeof(cl_test_system.regions[2].title),
           "Test Region 2 (little-endian)");

  cl_test_system.regions[3].base_guest = 0x40000000;
  cl_test_system.regions[3].size = region_size;
  cl_test_system.regions[3].base_host = malloc(region_size);
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

static cl_error cl_test(void)
{
  cl_search_t search;
  clock_t start, end;
  double cpu_time_used;
  int error;
  unsigned int word;
  unsigned char byte;
  unsigned i;
  
  /* Perform basic counter function tests */
  error = cl_ctr_tests();
  if (!error)
    printf("Counter tests passed!\n");
  else
  {
    printf("Counter test %d failed!\n", error);
    return error;
  }

  /* Register the test ABI */
  printf("Registering test ABI...\n");
  error = cl_abi_register(&cl_test_abi);
  if (error != CL_OK)
    return error;

  /* Initialize the test console */
  printf("Initializing test console...\n");
  error = cl_test_console_init();
  if (error != CL_OK)
    return error;

  /* Perform login flow tests */
  printf("Performing login flow tests...\n");
  error = cl_login_and_start(cl_test_system.identifier);
  if (error != CL_OK)
    return error;

  /* Perform some virtual memory tests */
  printf("Performing virtual memory tests...\n");
  word = 0xDEADBEEF;
  cl_write_memory(NULL, 0x30000000, sizeof(word), &word);
  cl_read_memory(&byte, NULL, 0x30000000, 1);
  if (byte != 0xEF)
  {
    printf("Little-endian virtual memory read/write test failed (got 0x%02x)!\n", byte);
    return CL_ERR_CLIENT_RUNTIME;
  }
  else
    printf("Little-endian virtual memory read/write test passed (got 0x%02x)!\n", byte);

  cl_write_memory(NULL, 0x40000000, sizeof(word), &word);
  cl_read_memory(&byte, NULL, 0x40000000, 1);
  if (byte != 0xDE)
  {
    printf("Big-endian virtual memory read/write test failed (got 0x%02x)!\n", byte);
    return CL_ERR_CLIENT_RUNTIME;
  }
  else
    printf("Big-endian virtual memory read/write test passed (got 0x%02x)!\n", byte);

  /* Initialize a search */
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
  printf("%lu matches found. %u pages. %lu memory usage. Time: %.6f s\n",
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
  printf("%lu matches found. %u pages. %lu memory usage. Time: %.6f s\n",
    search.total_matches, search.total_page_count, search.memory_usage, cpu_time_used);
  
  printf("Compare to 2...");
  word = 2;
  ((unsigned*)cl_test_system.regions[1].base_host)[0] = word;
  cl_search_change_target(&search, &word);
  start = clock();
  cl_search_step(&search);
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("%lu matches found. %u pages. %lu memory usage. Time: %.6f s\n",
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
  printf("%lu matches found. %u pages. %lu memory usage. Time: %.6f s\n",
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

  /* Run a few frames */
  printf("Running simulated frames...\n");
  word = 0x20000000;
  cl_write_memory(NULL, 0x10000000, sizeof(word), &word);
  for (i = 0; i < 10; i++)
  {
    cl_write_memory(NULL, 0x20000004, sizeof(i), &i);
    cl_run();
    cl_read_memory(&i, NULL, 0x20000004, sizeof(i));
    printf(" - Frame %u completed, memory at 0x20000004 = 0x%08x\n", i, i);
  }

  /* Close and free */
  printf("Freeing test session...\n");
  error = cl_free();
  if (error != CL_OK)
    return error;
  
  printf("Freeing test console...\n");
  error = cl_test_console_free();
  if (error != CL_OK)
    return error;

  printf("All tests completed successfully!\n");

  return CL_OK;
}

int main(void)
{
  unsigned i;

  for (i = 0; i < 10; i++)
  {
    printf("=== Running test iteration %u ===\n", i + 1);
    if (cl_test() != CL_OK)
      break;
    printf("\n");
  }

  return CL_OK;
}
