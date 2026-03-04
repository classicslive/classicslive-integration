#include "cl_search.h"

#include "cl_abi.h"
#include "cl_common.h"
#include "cl_memory.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static cl_error compare_to_nothing(cl_addr_t previous, cl_addr_t current, cl_compare_type type)
{
  switch (type)
  {
  case CL_COMPARE_EQUAL:
    return (previous == current) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_LESS:
  case CL_COMPARE_DECREASED:
    return (previous > current) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_GREATER:
  case CL_COMPARE_INCREASED:
    return (previous < current) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_NOT_EQUAL:
    return (previous != current) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  }

  return CL_ERR_PARAMETER_INVALID;
}

static cl_error compare_to_nothing_float(cl_addr_t previous, cl_addr_t current, cl_compare_type type)
{
  float fprevious, fcurrent;

  /* Cast to float */
  memcpy(&fprevious, &previous, sizeof(float));
  memcpy(&fcurrent,  &current,  sizeof(float));

  switch (type)
  {
  case CL_COMPARE_EQUAL:
    return ((uint32_t)fprevious == (uint32_t)fcurrent) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_LESS:
  case CL_COMPARE_DECREASED:
    return ((uint32_t)fprevious > (uint32_t)fcurrent) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_GREATER:
  case CL_COMPARE_INCREASED:
    return ((uint32_t)fprevious < (uint32_t)fcurrent) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_NOT_EQUAL:
    return ((uint32_t)fprevious != (uint32_t)fcurrent) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  }

  return CL_ERR_PARAMETER_INVALID;
}

static cl_error compare_to_value(cl_addr_t previous, cl_addr_t current, cl_compare_type type, cl_addr_t value)
{
  switch (type)
  {
  case CL_COMPARE_EQUAL:
    return (current == value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_GREATER:
    return (current > value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_LESS:
    return (current < value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_NOT_EQUAL:
    return (current != value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_INCREASED:
    return (current == previous + value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_DECREASED:
    return (current + value == previous) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  }

  return CL_ERR_PARAMETER_INVALID;
}

static cl_error compare_to_value_float(cl_addr_t previous, cl_addr_t current, cl_compare_type type,
  float value)
{
  float fprevious, fcurrent;
  cl_bool has_decimal_precision;

  /* Cast to float */
  memcpy(&fprevious, &previous, sizeof(float));
  memcpy(&fcurrent,  &current,  sizeof(float));

  /* This float is NaN */
  if (isnan(fcurrent))
    return CL_ERR_PARAMETER_INVALID;

  /* Only check decimal precision on equal ops if the user has specified */
  has_decimal_precision = floor(value) != value;

  switch (type)
  {
  case CL_COMPARE_EQUAL:
    if (has_decimal_precision)
      return (fcurrent == value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
    else
      return (floor(fcurrent) == value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_GREATER:
    return (fcurrent > value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_LESS:
    return (fcurrent < value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_NOT_EQUAL:
    return (fcurrent != value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_INCREASED:
    if (has_decimal_precision)
      return (fcurrent == fprevious + value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
    else
      return (floor(fcurrent) == floor(fprevious) + value) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  case CL_COMPARE_DECREASED:
    if (has_decimal_precision)
      return (fcurrent + value == fprevious) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
    else
      return (floor(fcurrent) + value == floor(fprevious)) ? CL_OK : CL_ERR_CLIENT_RUNTIME;
  }

  return CL_ERR_PARAMETER_INVALID;
}

static cl_error resolve_pointerresult(cl_addr_t *final_address, const cl_pointersearch_result_t *result,
  const unsigned passes)
{
  cl_addr_t address = result->address_initial;
  unsigned i;

  for (i = 0; i < passes; i++)
  {
    const cl_memory_region_t *region = cl_find_memory_region(address);
    cl_value_type ptr_type;

    if (!region)
      return CL_ERR_PARAMETER_NULL;

    ptr_type = cl_pointer_type(region->pointer_length);
    if (cl_read_memory_value(&address, NULL, address, ptr_type) != CL_OK)
      return CL_ERR_CLIENT_RUNTIME;

    address += result->offsets[i];
  }
  *final_address = address;

  return CL_OK;
}

cl_error cl_pointersearch_free(cl_pointersearch_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    free(search->results);
    return CL_OK;
  }
}

static cl_error add_pass(cl_pointersearch_t* search)
{
  cl_memory_region_t *region;
  cl_pointersearch_result_t *result;
  cl_addr_t matches, target, value;
  unsigned i, j, k, l;

  cl_pointersearch_result_t* new_results = (cl_pointersearch_result_t*)calloc(
    search->max_results, sizeof(cl_pointersearch_result_t));
  matches = 0;
  search->passes += 1;

  for (i = 0; i < search->result_count; i++)
  {
    cl_pointersearch_result_t *next_result = &search->results[i];

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

        if (value <= target && value >= target - search->range)
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
        if (matches == search->max_results)
        {
          cl_log("Search reached maximum count of %llu.\n", (unsigned long long)search->max_results);
          goto end;
        }
      }
    }
  }
  end:
  free(search->results);
  search->results = new_results;
  search->result_count = matches;

  return CL_OK;
}

cl_error cl_pointersearch_init(cl_pointersearch_t *search,
  cl_addr_t address, cl_value_type value_type, unsigned passes, cl_addr_t range,
  cl_addr_t max_results)
{
  cl_memory_region_t *region;
  cl_pointersearch_result_t *result;
  cl_addr_t matches, prev_value, value;
  unsigned i, j;

  if (!search || address == 0 || passes == 0)
    return CL_ERR_PARAMETER_INVALID;

  /* Is the address we're looking for valid? */
  if (cl_read_memory_value(&prev_value, NULL, address, value_type) != CL_OK)
  {
    cl_log("Address %016llX is invalid for a pointer search.\n", (unsigned long long)address);
    return CL_ERR_PARAMETER_INVALID;
  }

    /* Initialize search parameters */
    search->passes          = 1;
    search->range          = range;
    search->max_results    = max_results;
    search->params.compare_type = CL_COMPARE_EQUAL;
    search->params.value_size   = cl_sizeof_memtype(value_type);
    search->params.value_type   = value_type;

    /* We create a temporary array of max size and trim it down after */
    search->results = (cl_pointersearch_result_t*)calloc(
      max_results, sizeof(cl_pointersearch_result_t));
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
          cl_log("Pointer search for %016llX reached maximum result count of %llu.\n",
            (unsigned long long)address, (unsigned long long)max_results);

          return CL_OK;
        }
      }
    }
    search->result_count = matches;

    /* We've only done one pass so far. Run any extra passes */
    for (i = passes; i > 1; i--)
      add_pass(search);

    /* Clear the unneeded memory */
    search->result_count = matches;
    search->results = (cl_pointersearch_result_t*)realloc(
      search->results, matches * sizeof(cl_pointersearch_result_t));
#if CL_EXTERNAL_MEMORY
    free(region->base_host);
    region->base_host = NULL;
#endif

    cl_log("Pointer search for %016llX found %llu results.\n",
      (unsigned long long)address, (unsigned long long)matches);

    return CL_OK;
}

cl_addr_t cl_pointersearch_step(cl_pointersearch_t *search, const void *value)
{
  cl_pointersearch_result_t *result;
  cl_addr_t address;
  cl_addr_t matches, final_value, valid_pointers;
  cl_error compare_result;
  cl_compare_type cmp_type;
  unsigned i;

  if (!search)
    return 0;

  cmp_type = search->params.compare_type;
  matches = 0;
  valid_pointers = 0;
  cl_log("Result count at start: %llu\n", (unsigned long long)search->result_count);
  for (i = 0; i < search->result_count; i++)
  {
    result  = &search->results[i];

    if (resolve_pointerresult(&address, result, search->passes) != CL_OK)
      continue;
    else if (cl_read_memory_value(&final_value, NULL, address, search->params.value_type) != CL_OK)
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
          compare_to_value(result->value_previous, result->value_current, cmp_type, *((cl_addr_t*)value));
      }

      if (compare_result == CL_OK)
      {
        memcpy(&search->results[matches], result, sizeof(cl_pointersearch_result_t));
        matches++;
      }
      result->value_previous = result->value_current;
      valid_pointers++;
    }
  }
  /* All of the still valid results are grouped together, the rest of memory can be cleared */
  search->result_count = matches;
  search->results = (cl_pointersearch_result_t*)realloc(search->results, matches * sizeof(cl_pointersearch_result_t));
  cl_log("Pointer search now has %llu matches across %llu valid pointers.\n",
    (unsigned long long)matches, (unsigned long long)valid_pointers);

  return matches;
}

void cl_pointersearch_update(cl_pointersearch_t *search)
{
  cl_pointersearch_result_t *result;
  unsigned i;

  if (!search)
    return;

  for (i = 0; i < search->result_count; i++)
  {
    result = &search->results[i];

    if (resolve_pointerresult(&result->address_final, result, search->passes) != CL_OK)
      continue;
    else
      cl_read_memory_value(&result->value_current, NULL, result->address_final, search->params.value_type);
  }
}
