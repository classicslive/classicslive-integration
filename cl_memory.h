#ifndef CL_MEMORY_H
#define CL_MEMORY_H

#include "cl_common.h"

/* Memnotes marked with this are submitted to the server on every request,
   as well as on timed intervals to retrieve a play status string. */
#define CL_MEMFLAG_RICH 0

#include "cl_config.h"
#include "cl_counter.h"
#include "cl_types.h"

/**
 * Looks up which memory bank a given virtual address is contained in.
 * @param address A virtual memory address.
 * @return A pointer to the memory bank, or NULL if one is not found.
 */
cl_memory_region_t* cl_find_memory_region(cl_addr_t address);

/**
 * Frees all values contained within the global memory context.
 */
void cl_memory_free(void);

/**
 * Frees a memory note. Called automatically as part of cl_free_memory.
 * @param note The memory note to be freed.
 */
void cl_free_memnote(cl_memnote_t *note);

#if CL_LIBRETRO
/**
 * Initializes memory banks from an array of libretro memory descriptors,
 * informed via environment callback RETRO_ENVIRONMENT_SET_MEMORY_MAPS.
 * @param descs An array of libretro memory descriptors.
 * @param num_descs The count of elements in descs.
 * @return Whether or not memory banks could be initialized.
 */
struct retro_memory_descriptor;
bool cl_init_membanks_libretro(const struct retro_memory_descriptor **descs,
                               const unsigned num_descs);
#endif

/**
 * Checks whether or not a certain flag is set for a given memory note.
 * @param note A pointer to a memory note.
 * @param key A memory note key to be looked up automatically.
 * @param flag The memory note flag to check. For example, CL_MEMFLAG_RICH.
 **/
bool cl_get_memnote_flag(cl_memnote_t *note, uint8_t flag);
bool cl_get_memnote_flag_from_key(unsigned key, uint8_t flag);

/**
 * Copies the current value of a memory note into a buffer.
 * @param value A buffer for the value to be copied into. Should not be NULL.
 * @param note A pointer to a memory note.
 * @param key A memory note key to be looked up automatically.
 * @param type The data type of the buffer. For example, CL_MEMTYPE_32BIT.
 **/
bool cl_get_memnote_value(cl_counter_t *value, cl_memnote_t *note, unsigned type);
bool cl_get_memnote_value_from_key(cl_counter_t *value, unsigned key, unsigned type);

/* Populate a memory holder with values returned by the web API */
bool cl_init_memory(const char **pos);

/**
 * Reads a value at a virtual memory address into a buffer by reading internal
 *   process memory.
 * In most cases, the cl_read_memory macro should be used instead.
 * @param value The buffer to be read into.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual memory address to read from.
 * @param size The number of bytes to read.
 */
cl_error cl_read_memory_buffer_internal(void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_addr_t size);

/**
 * Reads a value at a virtual memory address into a buffer by reading internal
 *   process memory.
 * In most cases, the cl_read_memory macro should be used instead.
 * @param value The buffer to be read into.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual memory address to read from.
 * @param type The type of value to read.
 */
cl_error cl_read_memory_value_internal(void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_value_type type);

cl_error cl_write_memory_buffer_internal(const void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_addr_t size);

cl_error cl_write_memory_value_internal(const void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_value_type type);

#if CL_EXTERNAL_MEMORY
cl_error cl_read_memory_buffer_external(void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_addr_t size);
cl_error cl_read_memory_value_external(void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_value_type type);
cl_error cl_write_memory_buffer_external(const void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_addr_t size);
cl_error cl_write_memory_value_external(const void *buffer,
  const cl_memory_region_t *bank, cl_addr_t address, cl_value_type type);
#endif

#if CL_EXTERNAL_MEMORY
#define cl_read_memory_buffer cl_read_memory_buffer_external
#define cl_read_memory_value cl_read_memory_value_external
#define cl_write_memory_buffer cl_write_memory_buffer_external
#define cl_write_memory_value cl_write_memory_value_external
#else
#define cl_read_memory_buffer cl_read_memory_buffer_internal
#define cl_read_memory_value cl_read_memory_value_internal
#define cl_write_memory_buffer cl_write_memory_buffer_internal
#define cl_write_memory_value cl_write_memory_value_internal
#endif

/**
 * Steps through all memory notes and updates their values. Should be called 
 * once per frame.
 **/
void cl_update_memory(void);

/**
 * Writes the value referenced by a memory note with a given value.
 * @param note A pointer to a memory note.
 * @param key The unique key of a memory note.
 * @param value A buffer containing the source value.
 * @return Whether or not the write succeeded.
 **/
bool cl_write_memnote(cl_memnote_t *note, const cl_counter_t *value);
bool cl_write_memnote_from_key(unsigned key, const cl_counter_t *value);

/**
 * Looks up a memory note based on its key.
 * @param key The memory note key to look up. Currently a value between 0-9999.
 * @return A pointer to the appropriate memory note, or NULL if one with the
 * given key does not exist.
 **/
cl_memnote_t* cl_find_memnote(unsigned key);

/**
 * Adds a new memory note to the global memory context.
 * @param note The memory note to be added.
 * @return An error code representing the result of the operation.
 */
cl_error cl_memory_add_note(const cl_memnote_t *note);

/**
 * Verifies and initializes all memory notes in the global memory context.
 * Prints information about each note to the log.
 * @return An error code representing the result of the operation.
 */
cl_error cl_memory_init_notes(void);

extern cl_memory_t memory;

#endif
