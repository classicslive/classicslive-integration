#include "cl_search.h"

#include "cl_abi.h"
#include "cl_common.h"
#include "cl_memory.h"

#include <math.h>
#include <string.h>

bool compare_to_nothing(uint32_t previous, uint32_t current, uint8_t type)
{
  switch (type)
  {
  case CL_COMPARE_EQUAL:
    return previous == current;
  case CL_COMPARE_LESS:
  case CL_COMPARE_DECREASED:
    return previous > current;
  case CL_COMPARE_GREATER:
  case CL_COMPARE_INCREASED:
    return previous < current;
  case CL_COMPARE_NOT_EQUAL:
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
  case CL_COMPARE_EQUAL:
    return (uint32_t)fprevious == (uint32_t)fcurrent;
  case CL_COMPARE_LESS:
  case CL_COMPARE_DECREASED:
    return (uint32_t)fprevious > (uint32_t)fcurrent;
  case CL_COMPARE_GREATER:
  case CL_COMPARE_INCREASED:
    return (uint32_t)fprevious < (uint32_t)fcurrent;
  case CL_COMPARE_NOT_EQUAL:
    return (uint32_t)fprevious != (uint32_t)fcurrent;
  }

  return false;
}

bool compare_to_value(uint32_t previous, uint32_t current, uint8_t type, uint32_t value)
{
  switch (type)
  {
  case CL_COMPARE_EQUAL:
    return current == value;
  case CL_COMPARE_GREATER:
    return current > value;
  case CL_COMPARE_LESS:
    return current < value;
  case CL_COMPARE_NOT_EQUAL:
    return current != value;
  case CL_COMPARE_INCREASED:
    return current == previous + value;
  case CL_COMPARE_DECREASED:
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
  case CL_COMPARE_EQUAL:
    if (has_decimal_precision)
      return fcurrent == value;
    else
      return floor(fcurrent) == value;
  case CL_COMPARE_GREATER:
    return fcurrent > value;
  case CL_COMPARE_LESS:
    return fcurrent < value;
  case CL_COMPARE_NOT_EQUAL:
    return fcurrent != value;
  case CL_COMPARE_INCREASED:
    if (has_decimal_precision)
      return fcurrent == fprevious + value;
    else
      return floor(fcurrent) == floor(fprevious) + value;
  case CL_COMPARE_DECREASED:
    if (has_decimal_precision)
      return fcurrent + value == fprevious;
    else
      return floor(fcurrent) + value == floor(fprevious);
  }

  return false;
}

bool resolve_pointerresult(cl_addr_t *final_address, const cl_pointerresult_t *result, 
  const uint8_t passes)
{
  cl_addr_t address = result->address_initial;
  unsigned i;

  for (i = 0; i < passes; i++)
  {
    const cl_memory_region_t *region = cl_find_memory_region(address);
    cl_value_type ptr_type = cl_pointer_type(region->pointer_length);

    if (!region)
      return false;
    else if (!cl_read_memory_value(&address, NULL, address, ptr_type))
      return false;
    else
      address += result->offsets[i];
  }
  *final_address = address;

  return true;
}

bool cl_pointersearch_free(cl_pointersearch_t *search)
{
  if (!search)
    return false;
  else
  {
    free(search->results);
    return true;
  }
}

bool add_pass(cl_pointersearch_t* search, uint32_t range, uint32_t max_results)
{
  cl_memory_region_t *region;
  cl_pointerresult_t *result;
  uint32_t matches, target, value;
  uint32_t i, j, k, l;

  cl_pointerresult_t* new_results = (cl_pointerresult_t*)calloc(
    max_results, sizeof(cl_pointerresult_t));
  matches = 0;
  search->passes += 1;

  for (i = 0; i < search->result_count; i++)
  {
    cl_pointerresult_t *next_result = &search->results[i];

    for (j = 0; j < memory.region_count; j++)
    {
      region = &memory.regions[j];
      target = next_result->address_initial;

      if (region->size < region->pointer_length)
        continue;

      for (k = 0; k < region->size; k += region->pointer_length)
      {
        cl_value_type ptr_type = cl_pointer_type(region->pointer_length);
        cl_read_memory_value_internal(&value, region, k, ptr_type);

        if (value <= target && value >= target - range)
        {
          result = &new_results[matches];

          /* Shift all offsets over by one */
          for (l = search->passes - 1; l > 0; l--)
            result->offsets[l] = next_result->offsets[l - 1];

          /* Make this the new initial offset */
          result->offsets[0] = target - value;
          result->address_initial = region->base_guest + k;
          matches++;
        }

        /* Back out if we have too many results */
        if (matches == max_results)
        {
          cl_log("Search reached maximum count of %u.\n", max_results);
          goto end;
        }
      }
    }
  }
  end:
  free(search->results);
  search->results = new_results;
  search->result_count = matches;

  return true;
}

bool cl_pointersearch_init(cl_pointersearch_t *search, 
  cl_addr_t address, uint8_t val_type, uint8_t passes, uint32_t range,
  uint32_t max_results)
{
  if (!search || address == 0 || passes == 0)
    return false;
  else
  {
    cl_memory_region_t *region;
    cl_pointerresult_t *result;
    uint32_t matches, prev_value, value;
    uint32_t i, j;

    /* Is the address we're looking for valid? */
    if (!cl_read_memory_value(&prev_value, NULL, address, val_type))
    {
      cl_log("Address %08X is invalid for a pointer search.\n", address);
      return false;
    }

    /* Initialize search parameters */
    search->passes          = 1;
    search->range          = range;
    search->params.compare_type = CL_COMPARE_EQUAL;
    search->params.size      = cl_sizeof_memtype(val_type);
    search->params.value_type  = val_type;

    /* We create a temporary array of max size and trim it down after */
    search->results = (cl_pointerresult_t*)calloc(
      max_results, sizeof(cl_pointerresult_t));
    matches = 0;

    /* Do a quick scan to see how many results we start with */
    for (i = 0; i < memory.region_count; i++)
    {
      region = &memory.regions[i];

      if (region->size < region->pointer_length)
        continue;

#if CL_EXTERNAL_MEMORY
      region->base_host = malloc(region->size);
      cl_read_memory_buffer_external(region->base_host, NULL, region->base_guest, region->size);
#endif

      for (j = 0; j < region->size; j += region->pointer_length)
      {
        cl_value_type ptr_type = cl_pointer_type(region->pointer_length);
        cl_read_memory_value_internal(&value, region, j, ptr_type);

        if (value <= address && value >= address - range)
        {
          result = &search->results[matches];

          result->offsets[0]    = address - value;
          result->address_initial = region->base_guest + j;
          result->address_final  = address;
          result->value_current  = prev_value;
          result->value_previous  = prev_value;
          matches++;
        }

        if (matches == max_results)
        {
          search->result_count = max_results;
          cl_log("Pointer search for %08X reached maximum result count of %u.\n", address, max_results);

          return true;
        }
      }
    }
    search->result_count = matches;

    /* We've only done one pass so far. Run any extra passes */
    for (i = passes; i > 1; i--)
      add_pass(search, range, max_results);

    /* Clear the unneeded memory */
    search->result_count = matches;
    search->results = (cl_pointerresult_t*)realloc(
      search->results, matches * sizeof(cl_pointerresult_t));
#if CL_EXTERNAL_MEMORY
    free(region->base_host);
    region->base_host = NULL;
#endif

    cl_log("Pointer search for %08X found %u results.\n", address, matches);

    return true;
  }
}

uint32_t cl_pointersearch_step(cl_pointersearch_t *search, void *value)
{
  if (!search)
    return false;
  else
  {
    cl_pointerresult_t *result;
    cl_addr_t address;
    uint32_t matches, final_value, valid_pointers;
    bool compare_result;
    uint8_t cmp_type = search->params.compare_type;
    uint32_t i;

    matches = 0;
    valid_pointers = 0;
    cl_log("Result count at start: %u\n", search->result_count);
    for (i = 0; i < search->result_count; i++)
    {
      result  = &search->results[i];

      if (!resolve_pointerresult(&address, result, search->passes))
        continue;
      else if (!cl_read_memory_value(&final_value, NULL, address, search->params.value_type))
        continue;
      else
      {
        result->value_current = final_value;

        if (!value)
        {
          compare_result = search->params.value_type == CL_MEMTYPE_FLOAT ?
            compare_to_nothing_float(result->value_previous, result->value_current, cmp_type) :
            compare_to_nothing(result->value_previous, result->value_current, cmp_type);
        }
        else
        {
          compare_result = search->params.value_type == CL_MEMTYPE_FLOAT ?
            compare_to_value_float(result->value_previous, result->value_current, cmp_type, *((float*)value)) :
            compare_to_value(result->value_previous, result->value_current, cmp_type, *((uint32_t*)value));
        }

        if (compare_result)
        {
          memcpy(&search->results[matches], result, sizeof(cl_pointerresult_t));
          matches++;
        }
        result->value_previous = result->value_current;
        valid_pointers++;
      }
    }
    /* All of the still valid results are grouped together, the rest of memory can be cleared */
    search->result_count = matches;
    search->results = (cl_pointerresult_t*)realloc(search->results, matches * sizeof(cl_pointerresult_t));
    cl_log("Pointer search now has %u matches across %u valid pointers.\n", matches, valid_pointers);

    return matches;
  }
}

void cl_pointersearch_update(cl_pointersearch_t *search)
{
  if (!search)
    return;
  else
  {
    cl_pointerresult_t *result;
    uint32_t i;

    for (i = 0; i < search->result_count; i++)
    {
      result = &search->results[i];

      if (!resolve_pointerresult(&result->address_final, result, search->passes))
        continue;
      else
        cl_read_memory_value(&result->value_current, NULL, result->address_final, search->params.value_type);
    }
  }
}
