#ifndef CL_MEMORY_C
#define CL_MEMORY_C

#include "cl_common.h"
#include "cl_memory.h"

cl_memory_t memory;

cl_membank_t* cl_find_membank(uint32_t address)
{
   if (!memory.bank_count)
      return NULL;
   else if (memory.bank_count == 1)
      return &memory.banks[0];
   else
   {
      uint8_t i;

      for (i = 0; i < memory.bank_count; i++)
      {
         if (memory.banks[i].start <= address && address < memory.banks[i].start + memory.banks[i].size)
            return &memory.banks[i];
      }
   }

   return NULL;
}

void cl_free_memnote(cl_memnote_t *memnote)
{
   free(memnote->pointer_offsets);
   memnote->pointer_passes  = 0;
   memnote->pointer_offsets = NULL;
}

void cl_free_memory()
{
   uint32_t i;

   free(memory.banks);
   for (i = 0; i < memory.note_count; i++)
      cl_free_memnote(&memory.notes[i]);
   memory.banks = NULL;
   memory.notes = NULL;
}

bool cl_get_memnote_flag(uint32_t index, uint8_t flag)
{
   if (index >= memory.note_count)
      return false;
   else
      return (memory.notes[index].flags & (1 << flag)) != 0;
}

bool cl_get_memnote_float(float *value, uint32_t memnote_id, uint8_t type)
{
   if (memnote_id >= memory.note_count)
      return false;
   else
   {
      switch (type)
      {
      case CL_SRCTYPE_CURRENT_RAM:
         memcpy(value, &memory.notes[memnote_id].value_current, sizeof(float));
         break;
      case CL_SRCTYPE_PREVIOUS_RAM:
         memcpy(value, &memory.notes[memnote_id].value_previous, sizeof(float));
         break;
      case CL_SRCTYPE_LAST_UNIQUE_RAM:
         memcpy(value, &memory.notes[memnote_id].value_last_unique, sizeof(float));
         break;
      default:
         return false;
      }
   }

   return true;
}

bool cl_get_memnote_value(uint32_t *value, uint32_t memnote_id, uint8_t type)
{
   if (memnote_id >= memory.note_count)
      return false;
   else
   {
      switch (type)
      {
      case CL_SRCTYPE_CURRENT_RAM:
         *value = memory.notes[memnote_id].value_current;
         break;
      case CL_SRCTYPE_PREVIOUS_RAM:
         *value = memory.notes[memnote_id].value_previous;
         break;
      case CL_SRCTYPE_LAST_UNIQUE_RAM:
         *value = memory.notes[memnote_id].value_last_unique;
         break;
      default:
         return false;
      }
   }
   if (memory.notes[memnote_id].type == CL_MEMTYPE_FLOAT)
      *value = *((float*)value);
   
   return true;
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

bool cl_init_membanks()
{
   rarch_system_info_t* sys_info = runloop_get_system_info();
   uint8_t i;

   memory.bank_count = sys_info->mmaps.num_descriptors;

   /* No mmaps; we use retro_get_memory_data instead */
   if (memory.bank_count == 0)
   {
      retro_ctx_memory_info_t mem_info;

      mem_info.id = RETRO_MEMORY_SYSTEM_RAM;
      core_get_memory(&mem_info);

      /* Nothing here either, let's give up */
      if (!mem_info.data)
         return false;

      memory.banks = (cl_membank_t*)malloc(sizeof(cl_membank_t));
      memory.banks[0].data = (uint8_t*)mem_info.data;
      memory.banks[0].size = mem_info.size;
      memory.banks[0].start = 0;
      snprintf(memory.banks[0].title, sizeof(memory.banks[0].title), "%s", "retro_get_memory_data");
      memory.bank_count = 1;
      cl_log("Using retro_get_memory_data | %p - %08X bytes\n", memory.banks[0].data, memory.banks[0].size);

      return true;
   }
   else
   {
      memory.banks = (cl_membank_t*)calloc(memory.bank_count, sizeof(cl_membank_t));
      for (i = 0; i < memory.bank_count; i++)
      {
         memory.banks[i].data = (uint8_t*)sys_info->mmaps.descriptors[i].core.ptr;
         memory.banks[i].size = sys_info->mmaps.descriptors[i].core.len;
         memory.banks[i].start = sys_info->mmaps.descriptors[i].core.start;
         if (sys_info->mmaps.descriptors[i].core.addrspace)
            snprintf(memory.banks[i].title, sizeof(memory.banks[i].title), "%s", sys_info->mmaps.descriptors[i].core.addrspace);
         else
            snprintf(memory.banks[i].title, sizeof(memory.banks[i].title), "Memory bank %u/%u", i + 1, memory.bank_count);
      }

      cl_sort_membanks(memory.banks, memory.bank_count);
      for (i = 0; i < memory.bank_count; i++)
         cl_log("Bank %02X: 0x%08X | %08X bytes | %p | %s\n", i, memory.banks[i].start, memory.banks[i].size, memory.banks[i].data, memory.banks[i].title);

      return true;
   }
}

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

      if (!(cl_strto(pos, &new_memnote->address,        4, false) &&
            cl_strto(pos, &new_memnote->type,           1, false) &&
            cl_strto(pos, &new_memnote->flags,          1, false) &&
            cl_strto(pos, &new_memnote->pointer_passes, 1, false)))
         return false;
         
      cl_log("S: %u, P: %u, A: %08X",
       cl_sizeof_memtype(new_memnote->type),
       new_memnote->pointer_passes,
       new_memnote->address);

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

bool cl_read_memory(uint32_t *value, cl_membank_t *bank, uint32_t address, uint8_t size)
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

uint8_t cl_sizeof_memtype(const uint8_t memtype)
{
   switch (memtype)
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
   case CL_MEMTYPE_DOUBLE:
      return 8;
   }

   return 0;
}

void cl_update_memory()
{
   uint32_t      address;
   bool          error;
   cl_memnote_t *note;
   uint32_t      value;
   uint32_t      i;
   uint8_t       j;

   if (memory.bank_count == 0)
      cl_init_membanks();

   for (i = 0; i < memory.note_count; i++)
   {
      note    = &memory.notes[i];
      address = note->address;
      error   = false;
      value   = 0;

      /* The "previous" value is the value from the previous frame */
      note->value_previous = note->value_current;

      /* Handle values that require pointer follows */
      for (j = 0; j < note->pointer_passes; j++)
      {
         if (!cl_read_memory(&value, NULL, address, memory.pointer_size))
         {
            /* Pointer leads to an invalid location */
            error = true;
            break;
         }
         else
            address = value + note->pointer_offsets[j];
      }
      if (error)
         continue;

      /* Update memnote values based on their reported size */
      if (cl_read_memory(&value, NULL, address, cl_sizeof_memtype(note->type)))
         note->value_current = value;

      /* Logic for "last unique" values; the previous value will persist */
      if (note->value_previous != note->value_current)
         note->value_last_unique = note->value_previous;
   }
}

bool cl_write_memory(cl_membank_t *bank, uint32_t address, uint8_t size, 
   const void *value)
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
   if (bank->data)
   /* TODO: The address should be masked */
      return cl_write(bank->data, value, address, size, memory.endianness);
   
   return false;
}

#endif
