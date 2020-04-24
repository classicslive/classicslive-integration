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

/* ============================================================================
   Meant for libretro mmaps but is also used for retro_get_memory_data
   This info allows us to follow pointers across different memory regions.
============================================================================ */
typedef struct cl_membank_t
{
   uint8_t  *data;
   uint32_t  size;
   uint32_t  start;
} cl_membank_t;

/* ============================================================================
   A "memnote" is a point in core memory that corresponds
   with an observable in-game value. Instead of accessing specific addreses
   every frame we allocate memnotes and update their contents, then look 
   off of them for script conditions.

   Here, we keep track of a memnote's value on the current and previous
   frame, as well as the last unique value it had before it became
   what it currently is. 
============================================================================ */
typedef struct cl_memnote_t
{
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
   uint32_t      note_count;
   cl_membank_t *banks;
   uint8_t       bank_count;

   /* Metadata about the emulated console that tells us how to read its values */
   uint8_t       endianness;
   uint8_t       pointer_size;
} cl_memory_t;

void cl_free_memory(cl_memory_t *memory);
void cl_free_memnote(cl_memnote_t *memnote);

bool cl_get_memnote_flag(uint32_t note_flags, uint8_t flag);
bool cl_get_memnote_float(float *value, uint32_t memnote_id, uint8_t type);
bool cl_get_memnote_value(uint32_t *value, cl_memory_t *memory, 
   uint32_t memnote_id, uint8_t type);

/* Populate a memory holder with values returned by the web API */
bool cl_init_memory(const char **pos, cl_memory_t *memory);

/* Read a value from a virtual memory address, return true on success */
/* A pointer to a memory bank can be specified. If this is NULL the 
   exact address will be inferred from the memory info. */
bool cl_read_memory(uint32_t *value, cl_memory_t *memory, cl_membank_t *bank, 
   uint32_t address, uint8_t size);

/* Returns size (in bytes) of a given memory type ID */
uint8_t cl_sizeof_memtype(const uint8_t memtype);

/* Step through all memnotes and update their values, called once per frame */
void cl_update_memory(cl_memory_t *memory);

bool cl_write_memory(cl_memory_t *memory, cl_membank_t *bank,
   uint32_t address, uint8_t size, const void *value);

#endif
