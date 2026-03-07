#include "cl_search_pointer_new.h"

#include "cl_memory.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CL_POINTERSEARCH_CHUNK_SIZE CL_MB(16)

typedef union
{
  uint8_t  u8;
  int8_t   s8;
  uint16_t u16;
  int16_t  s16;
  uint32_t u32;
  int32_t  s32;
  int64_t  s64;
  float    fp;
  double   dfp;
} cl_search_target_impl_t;

#define CL_TARGET(target) ((cl_search_target_impl_t *)&(target))

#define PTRSEARCH_CMP(T, field, cur_p, prv_p, tgt_p, params) \
{ \
  const cl_search_target_impl_t *cur = (const cl_search_target_impl_t *)(cur_p); \
  const cl_search_target_impl_t *prv = (const cl_search_target_impl_t *)(prv_p); \
  const cl_search_target_impl_t *tgt = (const cl_search_target_impl_t *)(tgt_p); \
  switch ((params)->compare_type) \
  { \
  case CL_COMPARE_EQUAL:     return (params)->target_none ? cur->field == prv->field : cur->field == tgt->field; \
  case CL_COMPARE_GREATER:   return (params)->target_none ? cur->field >  prv->field : cur->field >  tgt->field; \
  case CL_COMPARE_LESS:      return (params)->target_none ? cur->field <  prv->field : cur->field <  tgt->field; \
  case CL_COMPARE_NOT_EQUAL: return (params)->target_none ? cur->field != prv->field : cur->field != tgt->field; \
  case CL_COMPARE_INCREASED: return (params)->target_none ? cur->field >  prv->field : cur->field == (T)(prv->field + tgt->field); \
  case CL_COMPARE_DECREASED: return (params)->target_none ? cur->field <  prv->field : cur->field == (T)(prv->field - tgt->field); \
  default: return CL_FALSE; \
  } \
}

static cl_bool pointersearch_passes(const cl_search_parameters_t *params,
  const void *current, const void *previous)
{
  switch (params->value_type)
  {
  case CL_MEMTYPE_UINT8:  PTRSEARCH_CMP(uint8_t,  u8,  current, previous, params->target_ptr, params)
  case CL_MEMTYPE_INT8:   PTRSEARCH_CMP(int8_t,   s8,  current, previous, params->target_ptr, params)
  case CL_MEMTYPE_UINT16: PTRSEARCH_CMP(uint16_t, u16, current, previous, params->target_ptr, params)
  case CL_MEMTYPE_INT16:  PTRSEARCH_CMP(int16_t,  s16, current, previous, params->target_ptr, params)
  case CL_MEMTYPE_UINT32: PTRSEARCH_CMP(uint32_t, u32, current, previous, params->target_ptr, params)
  case CL_MEMTYPE_INT32:  PTRSEARCH_CMP(int32_t,  s32, current, previous, params->target_ptr, params)
  case CL_MEMTYPE_INT64:  PTRSEARCH_CMP(int64_t,  s64, current, previous, params->target_ptr, params)
  case CL_MEMTYPE_FLOAT:  PTRSEARCH_CMP(float,    fp,  current, previous, params->target_ptr, params)
  case CL_MEMTYPE_DOUBLE: PTRSEARCH_CMP(double,   dfp, current, previous, params->target_ptr, params)
  default: return CL_FALSE;
  }
}

/**
 * Reads a value from the deep copy, loading the CL_POINTERSEARCH_CHUNK_SIZE-aligned
 * sub-region containing the address if it has not been cached yet.
 */
static cl_error deepcopy_read(cl_pointersearch_deepcopy_t *dc,
  void *out, cl_addr_t address, cl_value_type type)
{
  const cl_memory_region_t *region = cl_find_memory_region(address);
  cl_addr_t value_size = cl_sizeof_memtype(type);
  cl_addr_t chunk_start, chunk_size, region_offset;
  cl_pointersearch_deepcopy_chunk_t *chunk;
  unsigned i;

  if (!region || !region->pointer_length)
    return CL_ERR_PARAMETER_NULL;

  /* Align chunk start to CL_POINTERSEARCH_CHUNK_SIZE within the region */
  region_offset = address - region->base_guest;
  chunk_start   = region->base_guest + (region_offset / CL_POINTERSEARCH_CHUNK_SIZE) * CL_POINTERSEARCH_CHUNK_SIZE;
  chunk_size    = region->size - (chunk_start - region->base_guest);
  if (chunk_size > CL_POINTERSEARCH_CHUNK_SIZE)
    chunk_size = CL_POINTERSEARCH_CHUNK_SIZE;

  for (i = 0; i < dc->chunk_count; i++)
  {
    if (dc->chunks[i].start == chunk_start)
    {
      memcpy(out, (uint8_t *)dc->chunks[i].chunk + (address - chunk_start), value_size);
      return CL_OK;
    }
  }

  dc->chunks = realloc(dc->chunks, (dc->chunk_count + 1) * sizeof(*dc->chunks));
  if (!dc->chunks)
    return CL_ERR_PARAMETER_NULL;

  chunk        = &dc->chunks[dc->chunk_count++];
  chunk->start = chunk_start;
  chunk->size  = chunk_size;
  chunk->chunk = malloc(chunk_size);
  if (!chunk->chunk)
    return CL_ERR_PARAMETER_NULL;

  {
    cl_value_type ptr_type = cl_pointer_type(region->pointer_length);
    cl_addr_t     ptr_size = region->pointer_length;
    cl_addr_t     k;

    for (k = 0; k + ptr_size <= chunk_size; k += ptr_size)
      cl_read_memory_value((uint8_t *)chunk->chunk + k, NULL,
        chunk_start + k, ptr_type);
  }

  memcpy(out, (uint8_t *)chunk->chunk + (address - chunk_start), value_size);
  return CL_OK;
}

static void deepcopy_free(cl_pointersearch_deepcopy_t *dc)
{
  unsigned i;

  for (i = 0; i < dc->chunk_count; i++)
    free(dc->chunks[i].chunk);
  free(dc->chunks);
  dc->chunks      = NULL;
  dc->chunk_count = 0;
}

cl_error cl_pointersearch_free(cl_pointersearch_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;

  free(search->results);
  search->results      = NULL;
  search->result_count = 0;

  return CL_OK;
}

cl_error cl_pointersearch_init(cl_pointersearch_t *search, cl_addr_t address,
  cl_value_type value_type, unsigned passes, cl_addr_t range, cl_addr_t max_results)
{
  if (!search || !address || !passes)
    return CL_ERR_PARAMETER_INVALID;

  if (!cl_find_memory_region(address))
    return CL_ERR_PARAMETER_INVALID;

  free(search->results);
  search->results            = NULL;
  search->result_count       = 0;
  search->passes             = passes;
  search->range              = range;
  search->max_results        = max_results;
  search->params.value_type  = value_type;
  search->params.value_size  = cl_sizeof_memtype(value_type);
  search->params.compare_type = CL_COMPARE_EQUAL;
  search->params.target_none = 0;

  CL_TARGET(search->params.target)->s64 = (int64_t)address;
  search->params.target_ptr = CL_TARGET(search->params.target);

  return CL_OK;
}

cl_error cl_pointersearch_step(cl_pointersearch_t *search)
{
  cl_pointersearch_deepcopy_t deepcopy;
  cl_pointersearch_result_t  *new_results;
  cl_addr_t                   matches;
  cl_addr_t                   target;
  unsigned                    i, pass, r;

  if (!search || !search->params.target_ptr)
    return CL_ERR_PARAMETER_NULL;

  target = *(const cl_addr_t *)search->params.target_ptr;

  /* Subsequent steps: filter the existing results list */
  if (search->results)
  {
    deepcopy.chunks      = NULL;
    deepcopy.chunk_count = 0;
    matches = 0;

    for (i = 0; i < search->result_count; i++)
    {
      cl_pointersearch_result_t *result = &search->results[i];
      cl_addr_t address = result->address_initial;
      cl_bool failed = CL_FALSE;

      for (pass = 0; pass < search->passes; pass++)
      {
        const cl_memory_region_t *region = cl_find_memory_region(address);
        cl_addr_t next = 0;

        if (!region)
        {
          failed = CL_TRUE;
          break;
        }
        else if (deepcopy_read(&deepcopy, &next, address,
          cl_pointer_type(region->pointer_length)) != CL_OK)
        {
          failed = CL_TRUE;
          break;
        }

        address = next + result->offsets[pass];
      }

      if (failed)
        continue;

      result->address_final  = address;
      result->value_previous = result->value_current;
      cl_read_memory_value(result->value_current.raw, NULL, address, search->params.value_type);

      if (pointersearch_passes(&search->params,
        result->value_current.raw, result->value_previous.raw))
        memcpy(&search->results[matches++], result, sizeof(*result));
    }

    search->result_count = matches;
    search->results = realloc(search->results,
      matches * sizeof(cl_pointersearch_result_t));
    deepcopy_free(&deepcopy);
    return CL_OK;
  }

  /* First step: build the initial list by scanning all memory */
  deepcopy.chunks      = NULL;
  deepcopy.chunk_count = 0;

  search->results = calloc(search->max_results, sizeof(cl_pointersearch_result_t));
  if (!search->results)
    return CL_ERR_PARAMETER_NULL;

  /* First pass: scan all memory for pointers within range of target */
  matches = 0;
  for (i = 0; i < memory.region_count; i++)
  {
    cl_memory_region_t *region   = &memory.regions[i];
    cl_value_type       ptr_type = cl_pointer_type(region->pointer_length);
    cl_addr_t           ptr_size = region->pointer_length;
    cl_addr_t           k;

    if (!ptr_size || region->size < ptr_size)
      continue;

    for (k = 0; k + ptr_size <= region->size; k += ptr_size)
    {
      cl_addr_t value = 0;

      if (deepcopy_read(&deepcopy, &value, region->base_guest + k, ptr_type) != CL_OK)
        continue;

      if (value <= target && value >= target - search->range)
      {
        cl_pointersearch_result_t *result = &search->results[matches];
        result->address_initial = region->base_guest + k;
        result->address_final   = target;
        result->offsets[0]      = target - value;
        /* TODO: compare value at target against search params */
        matches++;

        /* We've found the maximum number of pointers */
        if (matches >= search->max_results)
          goto first_pass_done;
      }
    }
  }
  first_pass_done:
  search->result_count = matches;

  /* Subsequent passes: find pointers to each result's initial address */
  for (pass = 1; pass < search->passes; pass++)
  {
    new_results = calloc(search->max_results, sizeof(cl_pointersearch_result_t));
    if (!new_results)
      break;

    matches = 0;

    for (r = 0; r < search->result_count; r++)
    {
      cl_pointersearch_result_t *prev       = &search->results[r];
      cl_addr_t                  prev_addr  = prev->address_initial;
      unsigned                   j, l;

      for (j = 0; j < memory.region_count; j++)
      {
        cl_memory_region_t *region   = &memory.regions[j];
        cl_value_type       ptr_type = cl_pointer_type(region->pointer_length);
        cl_addr_t           ptr_size = region->pointer_length;
        cl_addr_t           k;

        if (!ptr_size || region->size < ptr_size)
          continue;

        for (k = 0; k + ptr_size <= region->size; k += ptr_size)
        {
          cl_addr_t value = 0;

          if (deepcopy_read(&deepcopy, &value, region->base_guest + k, ptr_type) != CL_OK)
            continue;

          if (value <= prev_addr && value >= prev_addr - search->range)
          {
            cl_pointersearch_result_t *result = &new_results[matches];

            for (l = pass; l > 0; l--)
              result->offsets[l] = prev->offsets[l - 1];
            result->offsets[0]      = prev_addr - value;
            result->address_initial = region->base_guest + k;
            result->address_final   = target;
            matches++;

            if (matches == search->max_results)
            {
              free(search->results);
              search->results      = new_results;
              search->result_count = matches;
              deepcopy_free(&deepcopy);
              return CL_OK;
            }
          }
        }
      }
    }

    free(search->results);
    search->results      = new_results;
    search->result_count = matches;
  }

  search->results = realloc(search->results,
    search->result_count * sizeof(cl_pointersearch_result_t));

  deepcopy_free(&deepcopy);
  return CL_OK;
}

cl_error cl_pointersearch_update(cl_pointersearch_t *search)
{
  unsigned i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else for (i = 0; i < search->result_count; i++)
  {
    cl_pointersearch_result_t *result = &search->results[i];
    cl_addr_t address = result->address_initial;
    unsigned pass;
    cl_bool failed = CL_FALSE;

    result->value_previous = result->value_current;

    for (pass = 0; pass < search->passes; pass++)
    {
      const cl_memory_region_t *region = cl_find_memory_region(address);

      if (!region)
      {
        failed = CL_TRUE;
        break;
      }
      else if (cl_read_memory_value(&address, NULL, address,
        cl_pointer_type(region->pointer_length)) != CL_OK)
      {
        failed = CL_TRUE;
        break;
      }

      address += result->offsets[pass];
    }

    if (failed)
      continue;

    result->address_final = address;
    cl_read_memory_value(result->value_current.raw, NULL, address,
      search->params.value_type);
  }

  return CL_OK;
}

cl_error cl_pointersearch_change_compare_type(cl_pointersearch_t *search,
  cl_compare_type compare_type)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (compare_type == CL_COMPARE_INVALID ||
           compare_type >= CL_COMPARE_SIZE)
    return CL_ERR_PARAMETER_INVALID;
  else
  {
    search->params.compare_type = compare_type;

    return CL_OK;
  }
}

cl_error cl_pointersearch_change_value_type(cl_pointersearch_t *search, cl_value_type type)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (type == CL_MEMTYPE_NOT_SET || type >= CL_MEMTYPE_SIZE)
    return CL_ERR_PARAMETER_INVALID;
  else
  {
    search->params.value_type = type;
    search->params.value_size = cl_sizeof_memtype(type);

    return CL_OK;
  }
}

cl_error cl_pointersearch_change_target(cl_pointersearch_t *search, const void *value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (!value)
    search->params.target_none = 1;
  else
  {
    cl_search_target_impl_t *target_impl = CL_TARGET(search->params.target);

    target_impl->s64 = 0;
    switch (search->params.value_size)
    {
    case 1: target_impl->s8  = *(const int8_t  *)value; break;
    case 2: target_impl->s16 = *(const int16_t *)value; break;
    case 4: target_impl->s32 = *(const int32_t *)value; break;
    case 8: target_impl->s64 = *(const int64_t *)value; break;
    default: return CL_ERR_PARAMETER_INVALID;
    }
    search->params.target_ptr  = target_impl;
    search->params.target_none = 0;
  }

  return CL_OK;
}

cl_error cl_pointersearch_change_target_int(cl_pointersearch_t *search, cl_addr_t value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_target_impl_t *target_impl = CL_TARGET(search->params.target);

    target_impl->s64 = 0;
    switch (search->params.value_size)
    {
    case 1: target_impl->s8  = (int8_t)value;  break;
    case 2: target_impl->s16 = (int16_t)value; break;
    case 4: target_impl->s32 = (int32_t)value; break;
    case 8: target_impl->s64 = (int64_t)value; break;
    default: return CL_ERR_PARAMETER_INVALID;
    }
    search->params.target_ptr  = target_impl;
    search->params.target_none = 0;

    return CL_OK;
  }
}

cl_error cl_pointersearch_change_target_float(cl_pointersearch_t *search, double value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_target_impl_t *target_impl = CL_TARGET(search->params.target);

    target_impl->s64 = 0;
    switch (search->params.value_size)
    {
    case 4: target_impl->fp  = (float)value; break;
    case 8: target_impl->dfp = value;        break;
    default: return CL_ERR_PARAMETER_INVALID;
    }
    search->params.target_ptr  = target_impl;
    search->params.target_none = 0;

    return CL_OK;
  }
}
