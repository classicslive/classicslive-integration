#ifndef CL_MEMORY_H
#define CL_MEMORY_H

/* Memnotes marked with this are submitted to the server on every request,
   as well as on timed intervals to retrieve a play status string. */
#define CL_MEMFLAG_RICH    0

enum
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
};

typedef enum
{
  CL_SRCTYPE_IMMEDIATE_INT = 0,
  CL_SRCTYPE_CURRENT_RAM,
  CL_SRCTYPE_PREVIOUS_RAM,
  CL_SRCTYPE_LAST_UNIQUE_RAM,
  CL_SRCTYPE_ROM,
  CL_SRCTYPE_COUNTER,
  CL_SRCTYPE_IMMEDIATE_FLOAT,

  CL_SRCTYPE_SIZE
} cl_src_t;

#define CLE_CMPTYPE_EQUAL     1
#define CLE_CMPTYPE_GREATER   2
#define CLE_CMPTYPE_LESS      3
#define CLE_CMPTYPE_NOT_EQUAL 4
#define CLE_CMPTYPE_INCREASED 5
#define CLE_CMPTYPE_DECREASED 6
#define CLE_CMPTYPE_ABOVE     7
#define CLE_CMPTYPE_BELOW     8

#include "cl_config.h"
#include "cl_counter.h"
#include "cl_types.h"

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
   cl_addr_t size;

   /* The virtual location of the first byte of this bank's data */
   cl_addr_t start;

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
   cl_addr_t address;
   cl_addr_t address_initial;

   /* TODO: Set these back to correct sizes */
   uint32_t flags;
   uint32_t type;

   /* Stored values */
   cl_counter_t current;
   cl_counter_t previous;
   cl_counter_t last_unique;

   /* For following pointers to get RAM values */
   int32_t  *pointer_offsets;
   uint32_t  pointer_passes;

#ifdef CL_HAVE_EDITOR
   /* Metadata for generated human-readable strings in Live Editor */
   /* TODO: Identifiers */
   char description [2048];
   bool edited;
   char title       [256];
#endif
} cl_memnote_t;

/**
 * The global struct that contains information related to the content's memory,
 * including user-defined memory notes.
 **/
typedef struct cl_memory_t
{
   cl_memnote_t *notes;
   unsigned      note_count;

   cl_membank_t *banks;
   unsigned      bank_count;

   /* The endianness of the content's memory. For example, CL_ENDIAN_LITTLE. */
   uint8_t endianness;

   /* How many bytes the content uses for virtual addresses. */
   uint8_t pointer_size;
} cl_memory_t;

/**
 * Looks up which memory bank a given virtual address is contained in.
 * @param address A virtual memory address.
 * @return A pointer to the memory bank, or NULL if one is not found.
 **/
cl_membank_t* cl_find_membank(cl_addr_t address);

/**
 * Frees all values contained within the global memory context.
 **/
void cl_memory_free(void);

/**
 * Frees a memory note. Called automatically as part of cl_free_memory.
 * @param note The memory note to be freed.
 **/
void cl_free_memnote(cl_memnote_t *note);

#if CL_LIBRETRO == true
/**
 * Initializes memory banks from an array of libretro memory descriptors.
 * @param descs An array of libretro memory descriptors.
 * @param num_descs The count of elements in descs.
 * @return Whether or not memory banks could be initialized.
 **/
struct retro_memory_descriptor;
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
 * @param type The data type of the buffer. For example, CL_MEMTYPE_32BIT.
 **/
bool cl_get_memnote_value(cl_counter_t *value, cl_memnote_t *note, unsigned type);
bool cl_get_memnote_value_from_key(cl_counter_t *value, unsigned key, unsigned type);

/* Populate a memory holder with values returned by the web API */
bool cl_init_memory(const char **pos);

/** 
 * Reads a value at a virtual memory address into a buffer by using the memory
 * bank data pointer.
 * @param value The buffer to be read into.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual memory address to read from.
 * @param size The number of bytes to read.
 **/
bool cl_read_memory_internal(void *value, cl_membank_t *bank, cl_addr_t address, 
   unsigned size);

/** 
 * Reads a value at a virtual memory address into a buffer by reading external
 * process memory.
 * @param value The buffer to be read into.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual memory address to read from.
 * @param size The number of bytes to read.
 **/
bool cl_read_memory_external(void *value, cl_membank_t *bank, cl_addr_t address, 
   unsigned size);

#if CL_EXTERNAL_MEMORY != true
#define cl_read_memory cl_read_memory_internal
#else
#define cl_read_memory cl_read_memory_external
#endif

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
void cl_update_memory(void);

/**
 * Writes the given data to a location in emulated virtual memory.
 * @param bank A pointer to a specific memory bank, or NULL to have it be 
 * looked up automatically.
 * @param address The virtual address to write to.
 * @param size The number of bytes to write.
 * @param value A pointer to the source data.
 * @return Whether or not the write succeeded.
 **/
bool cl_write_memory(cl_membank_t *bank, cl_addr_t address, uint8_t size, 
   const void *value);

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
 * @param key The memory note key to look up. Currently a value between 0-999.
 * @return A pointer to the appropriate memory note, or NULL if one with the
 * given key does not exist.
 **/
cl_memnote_t* cl_find_memnote(uint32_t key);

extern cl_memory_t memory;

#endif
