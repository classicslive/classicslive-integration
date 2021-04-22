#ifndef CL_IDENTIFY_H
#define CL_IDENTIFY_H

#include "cl_common.h"
#include <lrc_hash.h>

typedef struct cl_md5_ctx_t
{
   MD5_CTX   context;
   void     *data;
   uint32_t  size;
   char     *md5_final;
   uint8_t   md5_raw[16];
} cl_md5_ctx_t;

bool cl_identify(struct retro_game_info *info, char *checksum, retro_task_callback_t callback);
bool cl_read_from_file(const char *path, uint8_t **data, uint32_t *size);

#endif
