#ifndef CL_SEARCH_C
#define CL_SEARCH_C

#include <string.h>
#include "cl_common.h"
#include "cl_search.h"

cl_searchbank_t* cl_searchbank_from_address(cl_search_t *search, 
   uint32_t address)
{
   if (!search)
      return NULL;
   else
   {
      cl_searchbank_t *sbank;
      uint8_t i;

      for (i = 0; i < search->searchbank_count; i++)
      {
         sbank = &search->searchbanks[i];
         if (sbank->bank->start < address &&
             sbank->bank->start + sbank->bank->size > address)
            return sbank;
      }

      return NULL;
   }
}

bool cl_search_free(cl_search_t *search)
{
   if (!search)
      return false;
   else
   {
      uint8_t i;

      for (i = 0; i < search->searchbank_count; i++)
      {
         free(search->searchbanks[i].backup);
         free(search->searchbanks[i].valid);
      }
      free(search->searchbanks);

      return true;
   }
}

bool cl_search_init(cl_search_t *search)
{
   if (!memory.bank_count)
      return false;
   else
   {
      uint8_t i;

      cl_log("Initializing a new search...\n");
      search->matches          = 0;
      search->searchbank_count = memory.bank_count;
      search->searchbanks      = (cl_searchbank_t*)calloc(search->searchbank_count, sizeof(cl_searchbank_t));
      for (i = 0; i < search->searchbank_count; i++)
      {
         search->searchbanks[i].any_valid = true;
         search->searchbanks[i].bank   = &memory.banks[i];
         search->searchbanks[i].backup = (uint8_t*)malloc(memory.banks[i].size);
         search->searchbanks[i].valid  = (uint8_t*)malloc(memory.banks[i].size);
      }

      return true;
   }
}

bool cl_read_search(uint32_t *value, cl_search_t *search, cl_searchbank_t *sbank, uint32_t address, uint8_t size)
{
   if (!sbank)
   {
      if (memory.bank_count == 0)
         return false;
      else if (memory.bank_count == 1)
         sbank = &search->searchbanks[0];
      else
      {
         uint8_t i;

         for (i = 0; i < memory.bank_count; i++)
         {
            if ((address >= memory.banks[i].start) && 
                (address <  memory.banks[i].start + memory.banks[i].size))
            {
               sbank = &search->searchbanks[i];
               address -= sbank->bank->start;
            }
         }
      }
   }
   if (sbank->backup)
      return cl_read(value, sbank->backup, address, size, memory.endianness);

   return false;
}

void cl_search_remove(cl_search_t *search, uint32_t address)
{
   cl_searchbank_t *sbank = cl_searchbank_from_address(search, address);

   if (sbank)
   {
      sbank->valid[address - sbank->bank->start] = 0;
      search->matches--;
   }
}

bool cl_search_reset(cl_search_t *search)
{
   if (!search)
      return false;
   else
   {
      cl_searchbank_t *sbank;
      uint8_t i;

      for (i = 0; i < search->searchbank_count; i++)
      {
         sbank = &search->searchbanks[i];
         memcpy(sbank->backup, sbank->bank->data, sbank->bank->size);
         memset(sbank->valid, 1, sbank->bank->size);
         sbank->any_valid = true;
      }
      search->matches = 0;

      return true;
   }
}

bool compare_to_nothing(uint32_t previous, uint32_t current, uint8_t type)
{
   switch (type)
   {
   case CLE_CMPTYPE_EQUAL:
      return previous == current;
   case CLE_CMPTYPE_LESS:
   case CLE_CMPTYPE_DECREASED:
      return previous > current;
   case CLE_CMPTYPE_GREATER:
   case CLE_CMPTYPE_INCREASED:
      return previous < current;
   case CLE_CMPTYPE_NOT_EQUAL:
      return previous != current;
   }

   return false;
}

bool compare_to_nothing_float(uint32_t previous, uint32_t current, uint8_t type)
{
   float fprevious, fcurrent;

   /* Cast to float */
   memcpy(&fprevious, &previous, sizeof(float));
   memcpy(&fcurrent,  &current,  sizeof(float));

   switch (type)
   {
   case CLE_CMPTYPE_EQUAL:
      return (uint32_t)fprevious == (uint32_t)fcurrent;
   case CLE_CMPTYPE_LESS:
   case CLE_CMPTYPE_DECREASED:
      return (uint32_t)fprevious > (uint32_t)fcurrent;
   case CLE_CMPTYPE_GREATER:
   case CLE_CMPTYPE_INCREASED:
      return (uint32_t)fprevious < (uint32_t)fcurrent;
   case CLE_CMPTYPE_NOT_EQUAL:
      return (uint32_t)fprevious != (uint32_t)fcurrent;
   }

   return false;
}

bool compare_to_value(uint32_t previous, uint32_t current, uint8_t type, uint32_t value)
{
   switch (type)
   {
   case CLE_CMPTYPE_EQUAL:
      return current == value;
   case CLE_CMPTYPE_GREATER:
      return current > value;
   case CLE_CMPTYPE_LESS:
      return current < value;
   case CLE_CMPTYPE_NOT_EQUAL:
      return current != value;
   case CLE_CMPTYPE_INCREASED:
      return current == previous + value;
   case CLE_CMPTYPE_DECREASED:
      return current + value == previous;
   }

   return false;
}

bool compare_to_value_float(uint32_t previous, uint32_t current, uint8_t type, 
   float value)
{
   float fprevious, fcurrent;
   bool has_decimal_precision;

   /* Cast to float */
   memcpy(&fprevious, &previous, sizeof(float));
   memcpy(&fcurrent,  &current,  sizeof(float));

   /* This float is NaN */
   if (isnan(fcurrent))
      return false;

   /* Only check decimal precision on equal ops if the user has specified */
   has_decimal_precision = floor(value) != value;

   switch (type)
   {
   case CLE_CMPTYPE_EQUAL:
      if (has_decimal_precision)
         return fcurrent == value;
      else
         return floor(fcurrent) == value;
   case CLE_CMPTYPE_GREATER:
      return fcurrent > value;
   case CLE_CMPTYPE_LESS:
      return fcurrent < value;
   case CLE_CMPTYPE_NOT_EQUAL:
      return fcurrent != value;
   case CLE_CMPTYPE_INCREASED:
      if (has_decimal_precision)
         return fcurrent == fprevious + value;
      else
         return floor(fcurrent) == floor(fprevious) + value;
   case CLE_CMPTYPE_DECREASED:
      if (has_decimal_precision)
         return fcurrent + value == fprevious;
      else
         return floor(fcurrent) + value == floor(fprevious);
   }

   return false;
}

uint32_t cl_search_ascii(cl_search_t *search, const char *needle, uint8_t length)
{
   if (!search || search->searchbank_count == 0)
      return 0;
   else
   {
      cl_searchbank_t *sbank;
      const char *haystack;
      char *position;
      uint32_t matches = 0;
      uint8_t  i;
      uint32_t j;

      for (i = 0; i < search->searchbank_count; i++)
      {
         uint32_t matches_this_bank = 0;

         sbank = &search->searchbanks[i];
         memset(sbank->valid, 0, sbank->bank->size);
         haystack = (const char*)sbank->bank->data;
         
         for (j = 0; j < sbank->bank->size; j++)
         {
            if (!memcmp(&haystack[j], needle, length))
            {
               sbank->valid[j] = TRUE;
               matches_this_bank++;
            }
         }

         if (matches_this_bank == 0)
            sbank->any_valid = false;
         else
            matches += matches_this_bank;
         memcpy(sbank->backup, sbank->bank->data, sbank->bank->size);
      }
      search->matches = matches;

      return matches;
   }
}

uint32_t cl_search_step(cl_search_t *search, void *value, uint8_t size, 
   uint8_t type, bool is_float)
{
   if (!search || search->searchbank_count == 0)
      return 0;
   else
   {
      cl_searchbank_t *sbank;
      bool compare_result;
      uint32_t left, right;
      uint32_t matches = 0;
      uint8_t  i;
      uint32_t j;

      for (i = 0; i < search->searchbank_count; i++)
      {
         uint32_t matches_this_bank = 0;

         sbank = &search->searchbanks[i];
         if (!sbank->any_valid)
            continue;

         for (j = 0; j < sbank->bank->size; j += size)
         {
            /* This address has been weeded out already */
            if (!sbank->valid[j])
               continue;

            cl_read_search(&left, search, sbank, j, size);
            cl_read_memory(&right, sbank->bank, j, size);

            if (!value)
            {
               if (is_float)
                  compare_result = compare_to_nothing_float(left, right, type);
               else
                  compare_result = compare_to_nothing(left, right, type);
            }
            else
            {
               if (is_float)
                  compare_result = compare_to_value_float(left, right, type, *((float*)value));
               else
               {
                  uint32_t* intval = (uint32_t*)value;
                  
                  if (size == 1)
                     *intval &= 0xFF;
                  else if (size == 2)
                     *intval &= 0xFFFF;

                  compare_result = compare_to_value(left, right, type, *intval);
               }
            }  

            if (!compare_result)
               sbank->valid[j] = 0;
            else
            {
               sbank->valid[j] = 1;
               matches_this_bank++;
            }
         }

         if (matches_this_bank == 0)
            sbank->any_valid = false;
         else
            matches += matches_this_bank;
         memcpy(sbank->backup, sbank->bank->data, sbank->bank->size);
      }
      search->matches = matches;

      return matches;
   }
}

bool cl_pointersearch_free(cl_pointersearch_t *search)
{
   if (!search)
      return false;
   else
   {
      uint32_t i;

      for (i = 0; i < search->result_count; i++)
         free(search->results[i].offsets);
      free(search->results);

      return true;
   }
}

bool cl_pointersearch_init(cl_pointersearch_t *search, 
   uint32_t address, uint8_t size, uint8_t passes, uint32_t range, 
   uint32_t max_results)
{
   if (!search || address == 0 || passes == 0)
      return false;
   else
   {
      cl_membank_t *bank;
      cl_pointerresult_t *result;
      uint32_t current_match, matches, prev_value, value;
      bool exact_only;
      uint32_t i, j;

      /* Is the address we're looking for valid? */
      if (!cl_read_memory(&prev_value, NULL, address, size))
      {
         cl_log("Address %08X is invalid for a pointer search.\n", address);
         return false;
      }

      search->passes = passes;
      search->range  = range;
      search->size   = size;

      /* Only one memory bank; all pointed addresses are probably relative */
      if (memory.bank_count == 1)
         exact_only = false;

      /* We create a temporary array of max size and trim it down after */
      search->results = (cl_pointerresult_t*)calloc(max_results, sizeof(cl_pointerresult_t));
      matches = 0;

      /* Do a quick scan to see how many results we start with */
      for (i = 0; i < memory.bank_count; i++)
      {
         uint32_t target = exact_only ? bank->start + address : address;

         bank = &memory.banks[i];

         if (bank->size < memory.pointer_size)
            continue;

         for (j = 0; j < bank->size; j += memory.pointer_size)
         {
            cl_read_memory(&value, bank, j, memory.pointer_size);

            if (value <= target && value >= target - range)
            {
               result = &search->results[matches];

               result->offsets = (int32_t*)calloc(passes, sizeof(int32_t));
               result->offsets[0]      = address - value;
               result->address_initial = bank->start + j;
               result->address_final   = address;
               result->value_current   = prev_value;
               result->value_previous  = prev_value;
               matches++;
            }

            if (matches == max_results)
            {
               search->result_count = max_results;
               return true;
            }
         }
      }
      /* Clear the unneeded memory */
      search->result_count = matches;
      //search->results = realloc(search->results, matches * sizeof(cl_pointerresult_t));

      cl_log("Ptrsearch for %08X returned %u results.\n", address, matches);

      return true;
   }
}

uint32_t cl_pointersearch_step(cl_pointersearch_t *search, uint32_t *value,
   uint8_t size, uint8_t type)
{
   if (!search)
      return false;
   else
   {
      cl_pointerresult_t *result;
      uint32_t address, matches, final_value;
      bool compare_result;
      uint32_t i, j;

      matches = 0;
      for (i = 0; i < search->result_count; i++)
      {
         result  = &search->results[i];
         address = result->address_initial;

         for (j = 0; j < search->passes; j++)
            cl_read_memory(&address, NULL, address + result->offsets[j], memory.pointer_size);

         if (!cl_read_memory(&final_value, NULL, address, search->size))
            continue;
         else
         {
            if (!value)
               compare_result = compare_to_nothing(result->value_previous, result->value_current, type);
            else
               compare_result = compare_to_value(result->value_previous, result->value_current, type, *value);

            if (compare_result)
            {
               memcpy(&search->results[matches], result, sizeof(cl_pointerresult_t));
               matches++;
            }
         }
      }
      /* All of the still valid results are grouped together, the rest of memory can be cleared */
      search->result_count = matches;
      search->results = (cl_pointerresult_t*)realloc(search->results, matches);

      return matches;
   }
}

#endif
