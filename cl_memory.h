#ifndef CL_MEMORY_H
#define CL_MEMORY_H

/* Memnotes marked with this are submitted to the server on every request,
   as well as on timed intervals to retrieve a play status string. */
#define CL_MEMFLAG_RICH    0

#define CL_MEMTYPE_1BIT_A  1
#define CL_MEMTYPE_1BIT_B  2
#define CL_MEMTYPE_1BIT_C  3
#define CL_MEMTYPE_1BIT_D  4
#define CL_MEMTYPE_1BIT_E  5
#define CL_MEMTYPE_1BIT_F  6
#define CL_MEMTYPE_1BIT_G  7
#define CL_MEMTYPE_1BIT_H  8
#define CL_MEMTYPE_2BIT_A  9
#define CL_MEMTYPE_2BIT_B  10
#define CL_MEMTYPE_2BIT_C  11
#define CL_MEMTYPE_2BIT_D  12
#define CL_MEMTYPE_4BIT_LO 13
#define CL_MEMTYPE_4BIT_HI 14
#define CL_MEMTYPE_8BIT    15
#define CL_MEMTYPE_16BIT   16
#define CL_MEMTYPE_32BIT   17
#define CL_MEMTYPE_FLOAT   18
#define CL_MEMTYPE_DOUBLE  19

#define CL_SRCTYPE_IMMEDIATE       0
#define CL_SRCTYPE_CURRENT_RAM     1
#define CL_SRCTYPE_PREVIOUS_RAM    2
#define CL_SRCTYPE_LAST_UNIQUE_RAM 3
#define CL_SRCTYPE_ROM             4
#define CL_SRCTYPE_COUNTER         5

#define CLE_CMPTYPE_EQUAL     1
#define CLE_CMPTYPE_GREATER   2
#define CLE_CMPTYPE_LESS      3
#define CLE_CMPTYPE_NOT_EQUAL 4
#define CLE_CMPTYPE_INCREASED 5
#define CLE_CMPTYPE_DECREASED 6
#define CLE_CMPTYPE_ABOVE     7
#define CLE_CMPTYPE_BELOW     8

#include "cl_types.h"
#define HAVE_CLASSICS_LIVE_EDITOR true//TODO: remove

/**
 * A "memory bank" or "membank" is a region in emulated memory that has been
 * mapped virtually. This data allows us to follow pointers across different
 * memory regions.
**/
typedef struct cl_membank_t
{
   /* The location of the actual data */
   uint8_t *data;

   /* The number of bytes this bank contains */
   uint32_t size;

   /* The virtual location of the first byte of this bank's data */
   uint32_t start;

   char title[256];
} cl_membank_t;

/**
 * A "memory note" or "memnote" is a point in core memory that corresponds
 * with an observable in-game value. Instead of accessing specific addreses
 * every frame, we allocate memory notes and update their contents, then look 
 * off of them for script conditions.

 * Here, we keep track of a memnote's value on the current and previous
 * frame, as well as the last unique value it had before it became
 * what it currently is. 
**/
typedef struct cl_memnote_t
{
   uint32_t  key;
   uint32_t  order;
   uint32_t  address;

   /* TODO: Set these back to correct sizes */
   uint32_t  flags;
   uint32_t  type;

   /* Stored values */
   uint32_t  value_current;
   uint32_t  value_previous;
   uint32_t  value_last_unique;

   /* For following pointers to get RAM values */
   int32_t  *pointer_offsets;
   uint32_t  pointer_passes;

#ifdef HAVE_CLASSICS_LIVE_EDITOR
   /* Metadata for generated human-readable strings in Live Editor */
   /* TODO: Identifiers */
   char description [2048];
   bool edited;
   char title       [256];
#endif
} cl_memnote_t;

typedef struct cl_memory_t
{
   cl_memnote_t *notes;
   cl_membank_t *banks;
   unsigned      note_count;
   unsigned      bank_count;

   /* Metadata about the core that tells us how to read its values */
   uint8_t       endianness;
   uint8_t       pointer_size;
} cl_memory_t;

/**
 * Looks up which memory bank a given virtual address is contained in.
 * @param address A virtual memory address.
 * @return A pointer to the memory bank, or NULL if one is not found.
 **/
cl_membank_t* cl_find_membank(uint32_t address);

/**
 * Frees all values contained within the global memory context.
 **/
void cl_memory_free();

/**
 * Frees a memory note. Called automatically as part of cl_free_memory.
 * @param memnote The memory note to be freed
 **/
void cl_free_memnote(cl_memnote_t *note);

/**
 * Initializes memory banks from an array of libretro memory descriptors.
 * @param descs An array of libretro memory descriptors.
 * @param num_descs The count of elements in descs.
 * @return Whether or not memory banks could be initialized.
 **/
#ifdef LIBRETRO_H__
bool cl_init_membanks_libretro(struct retro_memory_descriptor **descs, 
   const unsigned num_descs);
#endif

/**
 * Checks whether or not a certain flag is set for a given memory note.
 * @param note A pointer to a memory note.
 * @param key A memory note key to be looked up automatically.
 * @param flag The memory note flag to check. For example, CL_MEMFLAG_RICH.
 **/
bool cl_get_memnote_flag(cl_memnote_t *note, uint8_t flag);
bool cl_get_memnote_flag_from_key(uint32_t key, uint8_t flag);

/**
 * Copies the current value of a memory note into a buffer.
 * @param value A buffer for the value to be copied into. Should not be NULL.
 * @param note A pointer to a memory note.
 * @param key A memory note key to be looked up automatically.
 * @param flag The memory note flag to check. For example, CL_MEMFLAG_RICH.
 **/
bool cl_get_memnote_value(uint32_t *value, cl_memnote_t *note, uint8_t type);
bool cl_get_memnote_value_from_key(uint32_t *value, uint32_t key, uint8_t type);

/* Populate a memory holder with values returned by the web API */
bool cl_init_memory(const char **pos);

/** 
 * Reads a value from a virtual memory address into a buffer.
 * @param value The buffer to be read into.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual memory address to read from.
 * @param size The number of bytes to read.
 **/
bool cl_read_memory(uint32_t *value, cl_membank_t *bank, uint32_t address, 
   uint8_t size);

/**
 * Returns the size (in bytes) of a given memory type ID.
 * @param type A type of memory value. For example, CL_MEMTYPE_8BIT.
 * @return The number of bytes the memory type takes up, or 0 if invalid.
 **/
unsigned cl_sizeof_memtype(const unsigned type);

/**
 * Steps through all memory notes and updates their values. Should be called 
 * once per frame.
 **/
void cl_update_memory();

/**
 * Writes the given data to a location in emulated virtual memory.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual address to write to.
 * @param size The number of bytes to write.
 * @param value A pointer to the source data.
 * @return Whether or not the write succeeded.
 **/
bool cl_write_memory(cl_membank_t *bank, uint32_t address, uint8_t size, 
   const void *value);

/* Writes the value referenced by a memnote with the given data */
bool cl_write_memnote(uint32_t key, const void *value);

extern cl_memory_t memory;

#endif
