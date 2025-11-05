#include "cl_common.h"
#include "cl_config.h"
#include "cl_frontend.h"
#include "cl_memory.h"

#if CL_LIBRETRO
#include <libretro.h>
#endif

#include <stdio.h>

cl_memory_t memory;

cl_memory_region_t* cl_find_memory_region(cl_addr_t address)
{
  if (memory.region_count == 0)
    return NULL;
  else if (memory.region_count == 1)
    return &memory.regions[0];
  else
  {
    cl_memory_region_t *region;
    unsigned i;

    for (i = 0; i < memory.region_count; i++)
    {
      region = &memory.regions[i];

      if (region->base_guest <= address &&
        address < region->base_guest + region->size)

      return region;
    }
  }

  return NULL;
}

cl_memnote_t* cl_find_memnote(unsigned key)
{
  unsigned i;

  for (i = 0; i < memory.note_count; i++)
  {
    if (memory.notes[i].key == key)
      return &memory.notes[i];
  }

  return NULL;
}

void cl_free_memnote(cl_memnote_t *note)
{
  /* Stubbed until something actually needs freeing */
  CL_UNUSED(note);
}

void cl_memory_free(void)
{
  unsigned i;

  for (i = 0; i < memory.note_count; i++)
    cl_free_memnote(&memory.notes[i]);
  memory.notes = NULL;

  free(memory.regions);
  memory.regions = NULL;
}

bool cl_get_memnote_flag(cl_memnote_t *note, uint8_t flag)
{
  if (!note)
    return false;
  else
    return (note->flags & (1 << flag)) != 0;
}

bool cl_get_memnote_flag_from_key(unsigned key, uint8_t flag)
{
  cl_memnote_t *note = cl_find_memnote(key);

  if (!note)
    return false;
  else
    return cl_get_memnote_flag(note, flag);
}

bool cl_get_memnote_value(cl_counter_t *src, cl_memnote_t *note, unsigned type)
{
  if (!src || !note)
   return false;
  else
  {
   switch (type)
   {
   case CL_SRCTYPE_CURRENT_RAM:
    *src = note->current;
    break;
   case CL_SRCTYPE_PREVIOUS_RAM:
    *src = note->previous;
    break;
   case CL_SRCTYPE_LAST_UNIQUE_RAM:
    *src = note->last_unique;
    break;
   default:
    return false;
   }

   return true;
  }
}

bool cl_get_memnote_value_from_key(cl_counter_t *src, unsigned key,
  unsigned type)
{
  cl_memnote_t *note = cl_find_memnote(key);

  if (!note)
   return false;
  else
   return cl_get_memnote_value(src, note, type);
}

void cl_sort_memory_regions(cl_memory_region_t *banks, unsigned count)
{
  unsigned i, j;

  for (i = 0; i < count; ++i)
    for (j = i + 1; j < count; ++j)
      if (banks[i].base_guest > banks[j].base_guest)
      {
        cl_memory_region_t temp = banks[i];

        banks[i] = banks[j];
        banks[j] = temp;
      }
}

#if CL_LIBRETRO
bool cl_init_membanks_libretro(const struct retro_memory_descriptor **descs,
  const unsigned num_descs)
{
  const struct retro_memory_descriptor *desc;
  unsigned i;

  memory.regions = (cl_memory_region_t*)calloc(num_descs,
                                               sizeof(cl_memory_region_t));
  memory.region_count = num_descs;

  for (i = 0; i < num_descs; i++)
  {
    desc = descs[i];
    cl_memory_region_t *region = &memory.regions[i];

    /* Is this bank's data a null pointer? Ignore it */
    if (!desc->ptr)
    {
      memory.region_count--;
      continue;
    }

    /* Copy the libretro mmap parameters into our format */
    region->base_alloc = CL_ADDRESS_INVALID;
    region->base_guest = desc->start;
    region->base_host = desc->ptr;
    region->endianness = desc->flags & RETRO_MEMDESC_BIGENDIAN ?
                                     CL_ENDIAN_BIG : CL_ENDIAN_LITTLE;
    region->flags = (cl_memory_region_flags){ .bits.read=1, .bits.write=1 };
    /**
     * @todo Is there a commonly used libretro flag for this? Not a huge deal
     * since this gets overwritten by the server later
     */
    region->pointer_length = 4;
    region->size = desc->len;

    /* Setup the title of the memory bank */
    if (desc->addrspace)
      snprintf(region->title, sizeof(region->title),
            "%s", desc->addrspace);
    else
      snprintf(region->title, sizeof(region->title),
            "Memory bank %u/%u", i + 1, memory.region_count);
  }

  cl_sort_memory_regions(memory.regions, memory.region_count);

  for (i = 0; i < memory.region_count; i++)
  {
    cl_memory_region_t *region = &memory.regions[i];

    cl_log("Bank %02X: 0x%08X | %08X bytes | %s\n", i, region->base_guest,
           region->size, region->title);
  }

  return true;
}
#endif

bool cl_init_memory(const char **pos)
{
  cl_memnote_t *new_memnote;
  uint32_t    i;

  if (!cl_strto(pos, &memory.note_count, sizeof(memory.note_count), false))
    return false;
  memory.notes = (cl_memnote_t*)calloc(memory.note_count, sizeof(cl_memnote_t));

  cl_log("Memory notes: %u\n", memory.note_count);

  for (i = 0; i < memory.note_count; i++)
  {
    cl_counter_t new_ctr;
    new_memnote = &memory.notes[i];

    if (!(cl_strto(pos, &new_memnote->key,        4, false) &&
        cl_strto(pos, &new_memnote->address_initial, sizeof(cl_addr_t), false) &&
        cl_strto(pos, &new_memnote->type,        4, false) &&
        cl_strto(pos, &new_memnote->flags,       4, false) &&
        cl_strto(pos, &new_memnote->pointer_passes, 4, false)))
      return false;

    cl_log("Memory note {%03u} - S: %u, P: %u, A: %08X",
     new_memnote->key,
     cl_sizeof_memtype(new_memnote->type),
     new_memnote->pointer_passes,
     new_memnote->address_initial);

    /* Initialize the tracked values based on the data type of the memnote */
    new_ctr.floatval.fp = 0;
    new_ctr.intval.i64 = 0;
    new_ctr.type = new_memnote->type;
    new_memnote->current    = new_ctr;
    new_memnote->previous   = new_ctr;
    new_memnote->last_unique = new_ctr;

    /* Initialize offsets for pointer-chain variables */
    if (new_memnote->pointer_passes > 0)
    {
      unsigned j;

      for (j = 0; j < new_memnote->pointer_passes; j++)
      {
        if (!cl_strto(pos, &new_memnote->pointer_offsets[j], sizeof(int32_t), true))
          return false;
        cl_log(" + %i", new_memnote->pointer_offsets[j]);
      }
    }
    cl_log("\n");
  }
  cl_log("End of memory.\n");

  return true;
}

cl_error cl_memory_add_note(const cl_memnote_t *note)
{
  cl_memnote_t *new_array;

  /* Attempt to reallocate with another element */
  new_array = (cl_memnote_t*)realloc(memory.notes,
    (memory.note_count + 1) * sizeof(cl_memnote_t));
  if (!new_array)
    return CL_ERR_CLIENT_RUNTIME;

  memory.notes = new_array;
  memory.notes[memory.note_count] = *note;
  memory.note_count++;

  cl_log("Added memnote {%03u} - S: %u, P: %u, A: %08X\n",
    note->key,
    cl_sizeof_memtype(note->type),
    note->pointer_passes,
    note->address_initial);

  return CL_OK;
}

unsigned cl_read_memory_internal(void *value, const cl_memory_region_t *bank,
  cl_addr_t address, unsigned size)
{
  if (!bank)
  {
    bank = cl_find_memory_region(address);
    if (!bank)
      return false;
    else
      address -= bank->base_guest;
  }
  if (bank->base_host && address < bank->size)
    return cl_read(value, bank->base_host, address, size, bank->endianness);

  return false;
}

#if CL_EXTERNAL_MEMORY
unsigned cl_read_memory_external(void *value, const cl_memory_region_t *bank,
  cl_addr_t address, unsigned size)
{
  CL_UNUSED(bank);
  return cl_fe_memory_read(&memory, value, address, size);
}
#endif

unsigned cl_sizeof_memtype(const unsigned type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
  case CL_MEMTYPE_UINT8:
    return 1;
  case CL_MEMTYPE_INT16:
  case CL_MEMTYPE_UINT16:
    return 2;
  case CL_MEMTYPE_INT32:
  case CL_MEMTYPE_UINT32:
  case CL_MEMTYPE_FLOAT:
    return 4;
  case CL_MEMTYPE_INT64:
  case CL_MEMTYPE_DOUBLE:
    return 8;
  }

  return 0;
}

/**
 * Gets the final address referenced by a memory note's chain of pointers, and
 * reads it into a buffer.
 * @param address The buffer to read the address into.
 * @param note A pointer to the memory note to have its address resolved.
 * @return Whether or not the final address could be inferred from the note.
 **/
bool cl_memnote_resolve_ptrs(cl_memnote_t *note)
{
  cl_addr_t final_addr = note->address_initial;
  unsigned i;

  for (i = 0; i < note->pointer_passes; i++)
  {
    const cl_memory_region_t *region = cl_find_memory_region(final_addr);

    if (!region)
      return false;
    else if (!cl_read_memory(&final_addr, NULL, final_addr,
                             region->pointer_length))
      return false;

    final_addr += note->pointer_offsets[i];
  }
  note->address = final_addr;

  return true;
}

bool cl_update_memnote(cl_memnote_t *note)
{
  if (!note || !cl_memnote_resolve_ptrs(note))
    return false;
  else
  {
    int64_t new_val = 0;

    /* The "previous" value is the value from the previous frame */
    note->previous = note->current;

    cl_read_memory(&new_val, NULL, note->address, cl_sizeof_memtype(note->type));
    cl_ctr_store(&note->current, &new_val, note->type);

    /* Logic for "last unique" values; the previous value will persist */
    if (!cl_ctr_equal_exact(&note->previous, &note->current))
      note->last_unique = note->previous;

    return true;
  }
}

void cl_update_memory(void)
{
  /* Have memory banks not been set up yet? */
  /* TODO: Maybe we should attempt to set up membanks here, like before */
  if (memory.region_count == 0)
    return;
  else
  {
    unsigned i;

    for (i = 0; i < memory.note_count; i++)
      cl_update_memnote(&memory.notes[i]);
  }
}

unsigned cl_write_memory(cl_memory_region_t *bank, cl_addr_t address,
                         unsigned size, const void *value)
{
#if CL_EXTERNAL_MEMORY
  CL_UNUSED(bank);
  return cl_fe_memory_write(&memory, value, address, size);
#else
  if (!size || !value)
    return false;
  else if (!bank)
  {
    bank = cl_find_memory_region(address);
    if (!bank)
      return false;
    else
      address -= bank->base_guest;
  }
  if (bank->base_host)
  /* TODO: The address should be masked */
    return cl_write(bank->base_host, value, address, size, bank->endianness);

  return false;
#endif
}

bool cl_write_memnote(cl_memnote_t *note, const cl_counter_t *value)
{
  if (!note || !value)
   return false;
  else
  {
   if (cl_memnote_resolve_ptrs(note))
   {
    if (cl_ctr_is_float(&note->current))
      return cl_write_memory(NULL,
                     note->address,
                     cl_sizeof_memtype(note->type),
                     &value->floatval) == cl_sizeof_memtype(note->type);
    else
      return cl_write_memory(NULL,
                     note->address,
                     cl_sizeof_memtype(note->type),
                     &value->intval) == cl_sizeof_memtype(note->type);
   }
  }

  return false;
}

bool cl_write_memnote_from_key(unsigned key, const cl_counter_t *value)
{
  cl_memnote_t *note = cl_find_memnote(key);

  return note ? cl_write_memnote(note, value) : false;
}
