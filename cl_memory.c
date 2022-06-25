#if CL_LIBRETRO == true
#include <libretro.h>
#endif

#include <string.h>

#include "cl_common.h"
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

/**
 * Looks up a memory note based on its key.
 * @param key The memory note key to look up. Currently a value between 0-999.
 * @return A pointer to the appropriate memory note, or NULL if one with the
 * given key does not exist.
 **/
cl_memnote_t* cl_find_memnote(uint32_t key)
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
   free(note->value_current);
   free(note->value_previous);
   free(note->value_last_unique);
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

bool cl_get_memnote_flag_from_key(uint32_t key, uint8_t flag)
{
   cl_memnote_t *note = cl_find_memnote(key);

   if (!note)
      return false;
   else
      return cl_get_memnote_flag(note, flag);
}

bool cl_cast(void *dst, void *src, unsigned dst_type, unsigned src_type)
{
   if (!dst || !src)
      return false;
   else if (dst_type == CL_MEMTYPE_32BIT)
   {
      uint32_t result;

      switch (src_type)
      {
      case CL_MEMTYPE_8BIT:
         result = (uint32_t)(*((uint8_t*)src));
         break;
      case CL_MEMTYPE_16BIT:
         result = (uint32_t)(*((uint16_t*)src));
         break;
      case CL_MEMTYPE_FLOAT:
         result = (uint32_t)(*((float*)src));
         break;
      default:
         return false;
      }
      *((uint32_t*)dst) = result;

      return true;
   }
   else if (dst_type == CL_MEMTYPE_FLOAT)
   {
      float result;

      switch (src_type)
      {
      case CL_MEMTYPE_8BIT:
         result = (float)(*((uint8_t*)src));
         break;
      case CL_MEMTYPE_16BIT:
         result = (float)(*((uint16_t*)src));
         break;
      case CL_MEMTYPE_32BIT:
         result = (float)(*((uint32_t*)src));
         break;
      default:
         return false;
      }
      *((float*)dst) = result;

      return true;
   }

   return false;
}

bool cl_get_memnote_value(void *value, cl_memnote_t *note, uint8_t type)
{
   if (!note)
      return false;
   else
   {
      switch (type)
      {
      case CL_SRCTYPE_CURRENT_RAM:
         return cl_cast(value, note->value_current, type, note->type);
      case CL_SRCTYPE_PREVIOUS_RAM:
         return cl_cast(value, note->value_previous, type, note->type);
      case CL_SRCTYPE_LAST_UNIQUE_RAM:
         return cl_cast(value, note->value_last_unique, type, note->type);
      }

      return false;
   }
}

bool cl_get_memnote_value_from_key(void *value, uint32_t key, uint8_t type)
{
   cl_memnote_t *note = cl_find_memnote(key);

   if (!note)
      return false;
   else
      return cl_get_memnote_value(value, note, type);
}

void cl_sort_membanks(cl_membank_t *banks, uint8_t count)
{
   uint8_t i, j;

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
      new_memnote = &memory.notes[i];

      if (!(cl_strto(pos, &new_memnote->key,            4, false) &&
            cl_strto(pos, &new_memnote->address, sizeof(cl_addr_t), false) &&
            cl_strto(pos, &new_memnote->type,           1, false) &&
            cl_strto(pos, &new_memnote->flags,          1, false) &&
            cl_strto(pos, &new_memnote->pointer_passes, 1, false)))
         return false;
         
      cl_log("Memory note {%03u} - S: %u, P: %u, A: %08X",
       new_memnote->key,
       cl_sizeof_memtype(new_memnote->type),
       new_memnote->pointer_passes,
       new_memnote->address);

      /* Allocate the tracked values based on the data type of the memnote */
      new_memnote->value_current     = calloc(1, cl_sizeof_memtype(new_memnote->type));
      new_memnote->value_previous    = calloc(1, cl_sizeof_memtype(new_memnote->type));
      new_memnote->value_last_unique = calloc(1, cl_sizeof_memtype(new_memnote->type));

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

bool cl_read_memory(void *value, cl_membank_t *bank, cl_addr_t address, uint8_t size)
{
   uint8_t i;

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

unsigned cl_sizeof_memtype(const unsigned type)
{
   switch (type)
   {
   case CL_MEMTYPE_8BIT:
   case CL_MEMTYPE_1BIT_A:
   case CL_MEMTYPE_1BIT_B:
   case CL_MEMTYPE_1BIT_C:
   case CL_MEMTYPE_1BIT_D:
   case CL_MEMTYPE_1BIT_E:
   case CL_MEMTYPE_1BIT_F:
   case CL_MEMTYPE_1BIT_G:
   case CL_MEMTYPE_1BIT_H:
   case CL_MEMTYPE_2BIT_A:
   case CL_MEMTYPE_2BIT_B:
   case CL_MEMTYPE_2BIT_C:
   case CL_MEMTYPE_2BIT_D:
   case CL_MEMTYPE_4BIT_LO:
   case CL_MEMTYPE_4BIT_HI:
      return 1;
   case CL_MEMTYPE_16BIT:
      return 2;
   case CL_MEMTYPE_32BIT:
   case CL_MEMTYPE_FLOAT:
      return 4;
   case CL_MEMTYPE_64BIT:
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
bool cl_memnote_resolve_ptrs(cl_addr_t *address, const cl_memnote_t *note)
{
   cl_addr_t final_addr = note->address;
   unsigned  i;

   for (i = 0; i < note->pointer_passes; i++)
   {
      if (!cl_read_memory(&final_addr, NULL, final_addr, memory.pointer_size))
         return false;
      final_addr += note->pointer_offsets[i];
   }
   *address = final_addr;

   return true;
}

void cl_update_memory(void)
{
   cl_memnote_t *note;
   cl_addr_t     address, address_tmp;
   unsigned      size;
   bool          error;
   unsigned      i, j;

   /* Have memory banks not been set up yet? */
   /* TODO: Maybe we should attempt to set up membanks here, like before */
   if (memory.bank_count == 0)
      return;

   for (i = 0; i < memory.note_count; i++)
   {
      note        = &memory.notes[i];
      address     = note->address;
      address_tmp = 0;
      size        = cl_sizeof_memtype(note->type);
      error       = false;

      /* The "previous" value is the value from the previous frame */
      memcpy(note->value_previous, note->value_current, size);

      /* Handle values that require pointer follows */
      for (j = 0; j < note->pointer_passes; j++)
      {
         if (!cl_read_memory(&address_tmp, NULL, address, memory.pointer_size))
         {
            /* Pointer leads to an invalid location */
            error = true;
            break;
         }
         else
            address = address_tmp + note->pointer_offsets[j];
      }
      if (error)
         continue;

      /* Update memnote values based on their reported size */
      cl_read_memory(note->value_current, NULL, address, size);

      /* Logic for "last unique" values; the previous value will persist */
      if (memcmp(note->value_previous, note->value_current, size))
         memcpy(note->value_last_unique, note->value_previous, size);
   }
}

bool cl_write_memory(cl_membank_t *bank, cl_addr_t address, uint8_t size, 
   const void *value)
{
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
}

bool cl_write_memnote(cl_memnote_t *note, const void *value)
{
   if (!note || !value)
      return false;
   else
   {
      cl_addr_t address;

      if (cl_memnote_resolve_ptrs(&address, note))
         return cl_write_memory(NULL, address, cl_sizeof_memtype(note->type), value);
   }

   return false;
}

bool cl_write_memnote_from_key(uint32_t key, const void *value)
{
   cl_memnote_t *note = cl_find_memnote(key);

   return note ? cl_write_memnote(note, value) : false;
}
