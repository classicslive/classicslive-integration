#if CL_LIBRETRO == true
#include <libretro.h>
#endif

#include <string.h>

#include "cl_common.h"
#include "cl_frontend.h"
#include "cl_memory.h"

cl_memory_t memory;

cl_membank_t* cl_find_membank(cl_addr_t address)
{
   if (!memory.bank_count)
      return NULL;
   else
   {
      cl_membank_t *bank;
      uint8_t i;

      for (i = 0; i < memory.bank_count; i++)
      {
         bank = &memory.banks[i];
         if (bank->start <= address && address < bank->start + bank->size)
            return bank;
      }
   }

   return NULL;
}

cl_memnote_t* cl_find_memnote(unsigned key)
{
   uint32_t i;

   for (i = 0; i < memory.note_count; i++)
   {
      if (memory.notes[i].key == key)
         return &memory.notes[i];
   }

   return NULL;
}

void cl_free_memnote(cl_memnote_t *note)
{
   free(note->pointer_offsets);
   note->pointer_passes  = 0;
   note->pointer_offsets = NULL;
}

void cl_memory_free()
{
   uint32_t i;

   free(memory.banks);
   for (i = 0; i < memory.note_count; i++)
      cl_free_memnote(&memory.notes[i]);
   memory.banks = NULL;
   memory.notes = NULL;
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

void cl_sort_membanks(cl_membank_t *banks, unsigned count)
{
  unsigned i, j;

  for (i = 0; i < count; ++i)
  {
    for (j = i + 1; j < count; ++j)
    {
      if (banks[i].start > banks[j].start)
      {
        cl_membank_t temp = banks[i];

        banks[i] = banks[j];
        banks[j] = temp;
      }
    }
  }
}

#if CL_LIBRETRO == true
bool cl_init_membanks_libretro(struct retro_memory_descriptor **descs, 
   const unsigned num_descs)
{
   const struct retro_memory_descriptor *desc;
   unsigned i;

   memory.banks = (cl_membank_t*)calloc(num_descs, sizeof(cl_membank_t));
   memory.bank_count = num_descs;
   for (i = 0; i < num_descs; i++)
   {
      desc = descs[i];

      /* Is this bank's data a null pointer? Ignore it */
      if (!desc->ptr)
      {
         memory.bank_count--;
         continue;
      }
      
      /* Copy the libretro mmap parameters into our format */
      memory.banks[i].data  = (uint8_t*)desc->ptr;
      memory.banks[i].size  = desc->len;
      memory.banks[i].start = desc->start;

      /* Setup the title of the memory bank */
      if (desc->addrspace)
         snprintf(memory.banks[i].title, sizeof(memory.banks[i].title),
                  "%s", desc->addrspace);
      else
         snprintf(memory.banks[i].title, sizeof(memory.banks[i].title),
                  "Memory bank %u/%u", i + 1, memory.bank_count);
   }

   cl_sort_membanks(memory.banks, memory.bank_count);
   for (i = 0; i < memory.bank_count; i++)
      cl_log("Bank %02X: 0x%08X | %08X bytes | %s\n", i,
             memory.banks[i].start,
             memory.banks[i].size,
             memory.banks[i].title);

   return true;
}
#endif

bool cl_init_memory(const char **pos)
{
   cl_memnote_t *new_memnote;
   uint32_t      i;
   
   if (!cl_strto(pos, &memory.note_count, sizeof(memory.note_count), false))
      return false;
   memory.notes = (cl_memnote_t*)calloc(memory.note_count, sizeof(cl_memnote_t));

   cl_log("Memory notes: %u\n", memory.note_count);

   for (i = 0; i < memory.note_count; i++)
   {
       cl_counter_t new_ctr;
      new_memnote = &memory.notes[i];

      if (!(cl_strto(pos, &new_memnote->key,            4, false) &&
            cl_strto(pos, &new_memnote->address_initial, sizeof(cl_addr_t), false) &&
            cl_strto(pos, &new_memnote->type,           1, false) &&
            cl_strto(pos, &new_memnote->flags,          1, false) &&
            cl_strto(pos, &new_memnote->pointer_passes, 1, false)))
         return false;
         
      cl_log("Memory note {%03u} - S: %u, P: %u, A: %08X",
       new_memnote->key,
       cl_sizeof_memtype(new_memnote->type),
       new_memnote->pointer_passes,
       new_memnote->address_initial);

      /* Initialize the tracked values based on the data type of the memnote */
      new_ctr.floatval = 0;
      new_ctr.intval = 0;
      new_ctr.type = new_memnote->type;
      new_memnote->current     = new_ctr;
      new_memnote->previous    = new_ctr;
      new_memnote->last_unique = new_ctr;

      /* Initialize offsets for pointer-chain variables */
      if (new_memnote->pointer_passes > 0)
      {
         uint8_t j;

         new_memnote->pointer_offsets = (int32_t*)calloc(new_memnote->pointer_passes, sizeof(int32_t));
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

   /* This will be overwritten, set it to something valid. */
   memory.endianness   = CL_ENDIAN_LITTLE;
   memory.pointer_size = 4;

   return true;
}

bool cl_read_memory_internal(void *value, cl_membank_t *bank, cl_addr_t address,
  unsigned size)
{
  if (!bank)
  {
    bank = cl_find_membank(address);
    if (!bank)
      return false;
    else
      address -= bank->start;
  }
  if (bank->data && address < bank->size)
    return cl_read(value, bank->data, address, size, memory.endianness);
   
  return false;
}

bool cl_read_memory_external(void *value, cl_membank_t *bank, cl_addr_t address, unsigned size)
{
#if CL_EXTERNAL_MEMORY == true
   return cl_fe_memory_read(&memory, value, address, size);
#else
   return false;
#endif
}

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
   unsigned  i;

   for (i = 0; i < note->pointer_passes; i++)
   {
      if (!cl_read_memory(&final_addr, NULL, final_addr, memory.pointer_size))
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

    /* Read the current frame's value from memory */
    cl_read_memory(&new_val, NULL, note->address, cl_sizeof_memtype(note->type));
    cl_ctr_store(&note->current, &new_val, note->type);

    /* Logic for "last unique" values; the previous value will persist */
    if (!cl_ctr_equal_exact(&note->previous, &note->current))
      note->last_unique = note->previous;
  }
}

void cl_update_memory(void)
{
  /* Have memory banks not been set up yet? */
  /* TODO: Maybe we should attempt to set up membanks here, like before */
  if (memory.bank_count == 0)
    return;
  else
  {
    unsigned i;

    for (i = 0; i < memory.note_count; i++)
      cl_update_memnote(&memory.notes[i]);
  }
}

bool cl_write_memory(cl_membank_t *bank, cl_addr_t address, uint8_t size, 
   const void *value)
{
#if CL_EXTERNAL_MEMORY == true
   return cl_fe_memory_write(&memory, value, address, size);
#else
   if (!size || !value)
      return false;
   else if (!bank)
   {
      bank = cl_find_membank(address);
      if (!bank)
         return false;
      else
         address -= bank->start;
   }
   if (bank->data)
   /* TODO: The address should be masked */
      return cl_write(bank->data, value, address, size, memory.endianness);
   
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
      if (note->type == CL_MEMTYPE_FLOAT || CL_MEMTYPE_DOUBLE)
        return cl_write_memory(NULL, note->address, cl_sizeof_memtype(note->type), &value->floatval);
      else
        return cl_write_memory(NULL, note->address, cl_sizeof_memtype(note->type), &value->intval);
    }
  }

  return false;
}

bool cl_write_memnote_from_key(unsigned key, const cl_counter_t *value)
{
  cl_memnote_t *note = cl_find_memnote(key);

  return note ? cl_write_memnote(note, value) : false;
}
