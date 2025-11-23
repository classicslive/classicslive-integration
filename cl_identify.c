#include "cl_config.h"
#include "cl_frontend.h"
#include "cl_identify.h"
#include "cl_memory.h"

#include <lrc_hash.h>
#include <retro_timers.h>
#include <string/stdstring.h>

#include <stdio.h>

#if CL_HAVE_FILESYSTEM
#include <streams/file_stream.h>
#include <streams/chd_stream.h>
#include <streams/interface_stream.h>
#include <file/file_path.h>
#endif

typedef struct cl_md5_ctx_t
{
  MD5_CTX   context;
  void     *data;
  unsigned  size;
  uint8_t   md5_raw[16];
  bool      free_on_finish;
  char     *md5_final;
} cl_md5_ctx_t;

#define CL_DOLPHIN_SIZE 0x002C
#define CL_ISO9660_SIZE 0x0800
#define CL_NCCH_SIZE    0x0200
#define CL_MAX_PATH     4096

static void cl_task_md5(struct cl_task_t *task)
{
  if (!task)
    return;
  else
  {
    cl_md5_ctx_t *state = (cl_md5_ctx_t*)task->state;

    MD5_Init(&state->context);
    MD5_Update(&state->context, state->data, state->size);
    MD5_Final(state->md5_raw, &state->context);

    snprintf(state->md5_final, 32 + 1, CL_SNPRINTF_MD5,
      state->md5_raw[0],  state->md5_raw[1],  state->md5_raw[2],
      state->md5_raw[3],  state->md5_raw[4],  state->md5_raw[5],
      state->md5_raw[6],  state->md5_raw[7],  state->md5_raw[8],
      state->md5_raw[9],  state->md5_raw[10], state->md5_raw[11],
      state->md5_raw[12], state->md5_raw[13], state->md5_raw[14],
      state->md5_raw[15]);

    cl_log("Content MD5: %.32s\n", state->md5_final);
    if (state->free_on_finish)
      free(state->data);
    free(state);
  }
}

static void cl_push_md5_task(void *data, unsigned size, char *checksum, 
  bool free_on_finish, CL_TASK_CB_T callback)
{
  cl_task_t *task = (cl_task_t*)calloc(1, sizeof(cl_task_t));
  cl_md5_ctx_t *context = (cl_md5_ctx_t*)calloc(1, sizeof(cl_md5_ctx_t));

  context->data           = data;
  context->size           = size;
  context->md5_final      = checksum;
  context->free_on_finish = free_on_finish;

  task->handler  = cl_task_md5;
  task->state    = context;
  task->callback = callback;

  cl_fe_thread(task);
}

/*
   Hash info loaded into the beginning of GC/Wii memory. (0x00 - 0x2B)
   This includes game ID, region, revision, and some console info.
*/
static void cl_task_gcwii(cl_task_t *task)
{
  if (!task)
    return;
  else
  {
    /* Give it an arbitrary amount of time to init fully */
    retro_sleep(5);

    cl_fe_install_membanks();
      
    /* When memory has been initialized, 0x20 in memory is 0D15EA5E. */
    if (memory.regions[0].base_host &&
        ((uint8_t*)(memory.regions[0].base_host))[0x20] == 0x0D &&
        ((uint8_t*)(memory.regions[0].base_host))[0x21] == 0x15 &&
        ((uint8_t*)(memory.regions[0].base_host))[0x22] == 0xEA &&
        ((uint8_t*)(memory.regions[0].base_host))[0x23] == 0x5E)
    {
      uint8_t *buffer;

      buffer = (uint8_t*)malloc(CL_DOLPHIN_SIZE);
      memcpy(buffer, memory.regions[0].base_host, CL_DOLPHIN_SIZE);
      cl_log("(GC/Wii) Game to be identified: %.8s\n", buffer);

      cl_push_md5_task(buffer, CL_DOLPHIN_SIZE,
        ((cl_md5_ctx_t*)task->state)->md5_final, true, task->callback);
      task->callback = NULL;
    }
  }
}

/**
 * Starts a task for hashing software running in Dolphin.
 * @param checksum A string to put the final hash in (32 bytes).
 * @param callback The function to call after the task finishes.
 */
static void cl_push_gcwii_task(char *checksum, CL_TASK_CB_T callback)
{
  cl_task_t *task = (cl_task_t*)calloc(1, sizeof(cl_task_t));
  cl_md5_ctx_t *context = (cl_md5_ctx_t*)calloc(1, sizeof(cl_md5_ctx_t));

  context->md5_final = checksum;

  task->handler  = cl_task_gcwii;
  task->state    = context;
  task->callback = callback;

  cl_fe_thread(task);
}

#if CL_HAVE_FILESYSTEM
bool cl_read_from_file(const char *path, uint8_t **data, uint32_t *size)
{
  uint8_t *buffer;
  int64_t  read_bytes;

  intfstream_t *stream = intfstream_open_file(path,
                                              RETRO_VFS_FILE_ACCESS_READ,
                                              RETRO_VFS_FILE_ACCESS_HINT_NONE);
         
  *size = (unsigned)intfstream_get_size(stream);
  buffer = (uint8_t*)malloc(*size);
  read_bytes = (int64_t)intfstream_read(stream, buffer, *size);
  intfstream_close(stream);
  if (!read_bytes)
  {
    free(buffer);
    *data = NULL;

    return false;
  }
  *data = buffer;

  return true;
}

/**
 * Hash the ISO9660 filesystem. Used for PS1, Saturn, Dreamcast, PS2, PSP, and
 *   other CD-based software.
 * @param stream
 * @return A buffer of size CL_ISO9660_SIZE containing the ISO9660 filesystem,
 *   or NULL if unavailable.
 * @todo For performance reasons we assume the CD001 identifer will be
 *   word-aligned. Would there ever be a reason where we need to scan
 *   byte-by-byte?
 * @todo Would we ever look at anything besides a primary volume descriptor?
 *   First byte would not be 0x01 then.
 **/
static uint8_t* cl_identify_iso9660(intfstream_t *stream)
{
  if (!stream)
    return NULL;
  else
  {
    uint8_t  *buffer;
    unsigned  size, i;

    buffer = (uint8_t*)malloc(CL_ISO9660_SIZE);
    size = (unsigned)intfstream_get_size(stream);

    /* Seek to the identifier "CD001" */
    for (i = 0; i < size; i += 8)
    {
      intfstream_read(stream, buffer, 8);
      if (buffer[0] == 0x01 && buffer[1] == 'C' &&
          buffer[2] == 'D'  && buffer[3] == '0' &&
          buffer[4] == '0'  && buffer[5] == '1')
      {
        cl_log("CD001 identifier found at 0x%08X\n", i);
        intfstream_read(stream, &buffer[8], CL_ISO9660_SIZE - 8);
        intfstream_close(stream);

        return buffer;
      }
    }
    /* Not found */
    intfstream_close(stream);
    free(buffer);

    return NULL;
  }
}

/**
 * Finds and returns the Nintendo Content Container Header. Used for 3DS.
 * @param path
 * @return A buffer of size CL_NCCH_SIZE containing the NCCH, or NULL if
 *   unsuccessful.
 * @todo See below and do this instead to support CIA.
 **/
static uint8_t* cl_identify_ncch(const char *path)
{
  intfstream_t *stream;
  uint8_t      *data;
  int64_t       read_bytes;

  stream = intfstream_open_file(path,
                                RETRO_VFS_FILE_ACCESS_READ,
                                RETRO_VFS_FILE_ACCESS_HINT_NONE);

  if (!stream)
    return NULL;

  /*
   * The NCCH header seems to always start 0x1000 bytes into the ROM.
   * If this isn't true we could instead search for the "NCCH" magic string
   * and seek backwards 0x0100.
   */
  intfstream_seek(stream, 0x1000, SEEK_SET);
  data = (uint8_t*)malloc(CL_NCCH_SIZE);
  read_bytes = intfstream_read(stream, data, CL_NCCH_SIZE);
  intfstream_close(stream);

  if (!read_bytes)
    free(data);
  else if (memcmp(&data[0x100], "NCCH", 4))
    cl_message(CL_MSG_ERROR, "Invalid NCCH data.");
  else
  {
    cl_log("NCCH product code: %s\n", &data[0x150]);
    return data;
  }

  return NULL;
}

/**
 * Opens the first track of a CHD file and returns the ISO9660 filesystem.
 * @param path
 * @return A buffer containing the ISO9660 filesystem, or NULL if unsuccessful.
 **/
static uint8_t* cl_identify_chd(const char *path)
{
  intfstream_t *stream;

  stream = intfstream_open_chd_track(path,
                                     RETRO_VFS_FILE_ACCESS_READ,
                                     RETRO_VFS_FILE_ACCESS_HINT_NONE,
                                     CHDSTREAM_TRACK_FIRST_DATA);
  if (!stream)
    return NULL;
  else
    return cl_identify_iso9660(stream);
}

/**
 * Opens a CUE file and determines the path of the first data track.
 * @param path Path to the CUE, overwritten by path to the first data track.
 * @param extension The extension of the first data track, in caps.
 * @return Whether or not the track data was properly read.
 **/
static bool cl_identify_cue(char *path, char *extension)
{
  const char *beginning;
  unsigned    length;
  char       *str;

  if (!cl_read_from_file(path, (uint8_t**)&str, &length))
    return false;
   
  beginning = strstr(str, "FILE ");
  if (beginning)
  {
    beginning += strlen("FILE ");
    if (*beginning == '"')
    {
      const char *end;

      beginning++;
      end = strstr(beginning, "\"") + 1;
      if (end)
      {
        unsigned filename_length;
        char     final[CL_MAX_PATH];
        char     path_temp[CL_MAX_PATH];

        /* Apply CUE pathname back to binary track */
        filename_length = end - beginning - 1;
        strncpy(final, beginning, filename_length);
        strncpy(path_temp, path, CL_MAX_PATH - 1);
        fill_pathname_resolve_relative(path, path_temp, final, sizeof(path_temp));

        /* Would extension lengths other than 3 ever be used? */
        snprintf(extension, 3, "%s", path_get_extension(path));
        string_to_upper(extension);
        cl_log("First data track of cue sheet: %s (%s)\n", path, extension);

        return true;
      }
      else
        cl_log("Malformed cue sheet (no ending quote on first data track).\n");
    }
    else
      cl_log("Malformed cue sheet (first data track isn't quote-wrapped).\n");
  }
  else
    cl_log("Malformed cue sheet (first data track not found).\n");

  return false;
}

/**
 * Opens an M3U playlist and determines the path of the first item.
 * @param path Path to the M3U, overwritten by path to the first item.
 * @param extension The extension of the first item, in caps.
 * @return Whether or not the first item was properly read.
 **/
bool cl_identify_m3u(char *path, char *extension)
{
  unsigned  length, i;
  char     *str;

  if (!cl_read_from_file(path, (uint8_t**)&str, &length))
    return false;

  for (i = 0; i < length; i++)
  {
    if (str[i] == '\r' || str[i] == '\n')
    {
      str[i] = '\0';
      fill_pathname_resolve_relative(path, path, str, CL_MAX_PATH);
      strcpy(extension, path_get_extension(path));
      string_to_upper(extension);
      cl_log("First item in M3U playlist: %s (%s)\n", path, extension);
      free(str);

      return true;
    }
  }
  cl_log("Malformed M3U.\n");
  free(str);

  return false;
}
#endif

#if CL_HAVE_FILESYSTEM
bool cl_identify(const void *info_data, const unsigned info_size,
  const char *info_path, const char *library, char *checksum, CL_TASK_CB_T callback)
{
  uint8_t  *data = NULL;
  char      extension[16];
  char      path[CL_MAX_PATH];
  unsigned  size = 0;

  strncpy(path, info_path, sizeof(path));
  strncpy(extension, path_get_extension(path), sizeof(extension));
  strncpy(extension, string_to_upper(extension), sizeof(extension));

  /*
    Hashing GC or Wii discs uses a background task that waits until the
    software has booted. Homebrew and Wii channels can be hashed as normal.
  */
  if (strstr(library, "dolphin") ||
    strstr(library, "ishiiruka"))
  {
    if (string_is_equal(extension, "ISO") ||
        string_is_equal(extension, "CSO") ||
        string_is_equal(extension, "GCZ") ||
        string_is_equal(extension, "GCM") ||
        string_is_equal(extension, "WBFS"))
    {
      cl_push_gcwii_task(checksum, callback);

      return true;
    }
  }

  /*
    Get the name of the file we actually need to verify.
    An M3U might point to a CUE, so we check both.
  */
  if (string_is_equal(extension, "M3U"))
    cl_identify_m3u(path, extension);
  if (string_is_equal(extension, "CUE"))
    cl_identify_cue(path, extension);

  cl_log("Final file to be identified: %s (%s)\n", path, extension);

  if (string_is_equal(extension, "BIN") ||
      string_is_equal(extension, "ECM") ||
      string_is_equal(extension, "ISO") ||
      string_is_equal(extension, "PBP") ||
      string_is_equal(extension, "CSO"))
  {
    intfstream_t *stream = intfstream_open_file(path,
                                                RETRO_VFS_FILE_ACCESS_READ,
                                                RETRO_VFS_FILE_ACCESS_HINT_NONE);

    data = cl_identify_iso9660(stream);
    size = CL_ISO9660_SIZE;
  }
  else if (string_is_equal(extension, "CHD"))
  {
    data = cl_identify_chd(path);
    size = CL_ISO9660_SIZE;
  }
  else if (strstr(library, "Citra"))
  {
    data = cl_identify_ncch(path);
    size = CL_NCCH_SIZE;
  }

  /* None of the quirks apply */
  if (!data)
  {
    /*
      File type wasn't recognized, we use the ROM already in memory
      if possible (most common case)
    */
    if (info_data && info_size > 0)
    {
      data = (uint8_t*)malloc(info_size);
      size = info_size;
      memcpy(data, info_data, size);
    }
    /*
      Last case: File was unrecognizable and not already in memory.
      Re-open the file using the path from retro_game_info and hash it.
    */
    else if (!cl_read_from_file(path, &data, &size))
      return false;
  }
  cl_push_md5_task(data, size, checksum, true, callback);

  return true;
}
#else
bool cl_identify(const void *info_data, const unsigned info_size,
  const char *info_path, const char *library, char *checksum, CL_TASK_CB_T callback)
{
  CL_UNUSED(info_path);
  CL_UNUSED(library);
  if (info_data && info_size > 0)
  {
    cl_push_md5_task(info_data, info_size, checksum, false, callback);
    return true;
  }
  else
    return false;
}
#endif
