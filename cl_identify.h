#ifndef CL_IDENTIFY_H
#define CL_IDENTIFY_H

#include "cl_common.h"

/**
 * Identifies the loaded content and prints a checksum into a provided buffer.
 * Can either be provided a buffer containing the content's data already in
 * memory, or the location of a file to identify.
 * @param info_data A pointer to the content's data in memory, or NULL.
 * @param info_size The size of the content, if using info_data.
 * @param info_path The location of a file to identify, if not using info_data.
 * @param library The name of the core, which can be used to choose more
 * specific identification methods.
 * @param callback A function to run after identification is complete.
 **/
bool cl_identify(const void *info_data, const unsigned info_size,
   const char *info_path, const char *library, char *checksum, void *callback);

bool cl_read_from_file(const char *path, uint8_t **data, uint32_t *size);

#endif
