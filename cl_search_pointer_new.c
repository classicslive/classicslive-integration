#include "cl_search_pointer_new.h"

#include "cl_memory.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
} cl_pointersearch_target_impl_t;

#define CL_POINTERSEARCH_TARGET(target) ((cl_pointersearch_target_impl_t *)&(target))

#define PTRSEARCH_CMP(T, field, cur_p, prv_p, tgt_p, params) \
{ \
  const cl_pointersearch_target_impl_t *cur = (const cl_pointersearch_target_impl_t *)(cur_p); \
  const cl_pointersearch_target_impl_t *prv = (const cl_pointersearch_target_impl_t *)(prv_p); \
  const cl_pointersearch_target_impl_t *tgt = (const cl_pointersearch_target_impl_t *)(tgt_p); \
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

static cl_bool pointersearch_passes(const cl_pointersearch_params_t *params,
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
  cl_value_type value_type, unsigned passes, cl_addr_t range,
  cl_addr_t max_results, cl_addr_t max_results_per_pass)
{
  if (!search || !address || !passes)
    return CL_ERR_PARAMETER_INVALID;

  if (!cl_find_memory_region(address))
    return CL_ERR_PARAMETER_INVALID;

  free(search->results);
  search->results              = NULL;
  search->result_count         = 0;
  search->passes               = passes;
  search->range                = range;
  search->max_results          = max_results;
  search->max_results_per_pass = max_results_per_pass;
  search->target_address       = address;
  search->params.value_type  = value_type;
  search->params.value_size  = cl_sizeof_memtype(value_type);
  search->params.compare_type = CL_COMPARE_EQUAL;
  search->params.target_none = 1;

  search->params.target_ptr = CL_POINTERSEARCH_TARGET(search->params.target);

  return CL_OK;
}

static cl_error add_pass(cl_pointersearch_t *search, unsigned pass, uint8_t *chunk)
{
  cl_pointersearch_result_t *new_results;
  cl_addr_t matches;
  unsigned  i, j, l;

  new_results = calloc(search->max_results, sizeof(cl_pointersearch_result_t));
  if (!new_results)
    return CL_ERR_PARAMETER_NULL;

  matches = 0;

  for (i = 0; i < search->result_count; i++)
  {
    cl_pointersearch_result_t *prev              = &search->results[i];
    cl_addr_t                  target            = prev->address_initial;
    cl_addr_t                  per_result_matches = 0;

    for (j = 0; j < memory.region_count; j++)
    {
      cl_memory_region_t *region   = &memory.regions[j];
      cl_value_type       ptr_type = cl_pointer_type(region->pointer_length);
      cl_addr_t           ptr_size = region->pointer_length;
      cl_addr_t           off;

      if (!ptr_size || region->size < ptr_size)
        continue;

      for (off = 0; off < region->size; off += CL_POINTERSEARCH_CHUNK_SIZE)
      {
        cl_addr_t chunk_size = region->size - off;
        cl_addr_t k;

        if (chunk_size > CL_POINTERSEARCH_CHUNK_SIZE)
          chunk_size = CL_POINTERSEARCH_CHUNK_SIZE;

        if (cl_read_memory_buffer(chunk, region, off, chunk_size) != CL_OK)
          continue;

        search->memory_scanned += chunk_size;

        for (k = 0; k + ptr_size <= chunk_size; k += ptr_size)
        {
          cl_addr_t value = 0;
          cl_read_value(&value, chunk, k, ptr_type, region->endianness);

          if (value <= target && value >= target - search->range)
          {
            cl_pointersearch_result_t *result = &new_results[matches];

            /* Shift all offsets over by one */
            for (l = pass; l > 0; l--)
              result->offsets[l] = prev->offsets[l - 1];

            /* Make this the new initial offset */
            result->offsets[0]      = target - value;
            result->address_initial = region->base_guest + off + k;
            result->address_final   = search->target_address;
            matches++;
            per_result_matches++;

            if (matches >= search->max_results)
              goto end;

            if (search->max_results_per_pass &&
                per_result_matches >= search->max_results_per_pass)
              goto next_result;
          }
        }
      }
    }
    next_result:;
  }
  end:
  free(search->results);
  search->results      = new_results;
  search->result_count = matches;

  return CL_OK;
}

cl_error cl_pointersearch_step(cl_pointersearch_t *search)
{
  cl_addr_t matches;
  unsigned  i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;

  /* Subsequent steps: resolve, update values, filter by comparison and validity */
  if (search->results)
  {
    clock_t start = clock();
    matches = 0;

    for (i = 0; i < search->result_count; i++)
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

      if (pointersearch_passes(&search->params,
        result->value_current.raw, result->value_previous.raw))
        memcpy(&search->results[matches++], result, sizeof(*result));
    }

    search->result_count  = matches;
    search->memory_scanned = 0;
    search->memory_usage  = matches * sizeof(cl_pointersearch_result_t);
    search->time_taken    = (double)(clock() - start) / CLOCKS_PER_SEC;
    search->results = realloc(search->results,
      matches * sizeof(cl_pointersearch_result_t));
    return CL_OK;
  }

  /* First step: scan all memory for pointer chains leading to target_address */
  {
    cl_addr_t  target_addr  = search->target_address;
    uint8_t   *chunk        = malloc(CL_POINTERSEARCH_CHUNK_SIZE);
    clock_t    start        = clock();
    unsigned   pass;

    if (!chunk)
      return CL_ERR_PARAMETER_NULL;

    search->results = calloc(search->max_results, sizeof(cl_pointersearch_result_t));
    if (!search->results)
    {
      free(chunk);
      return CL_ERR_PARAMETER_NULL;
    }

    search->memory_scanned = 0;

    /* Pass 0: find all pointers within range of target_address */
    matches = 0;
    for (i = 0; i < memory.region_count; i++)
    {
      cl_memory_region_t *region   = &memory.regions[i];
      cl_value_type       ptr_type = cl_pointer_type(region->pointer_length);
      cl_addr_t           ptr_size = region->pointer_length;
      cl_addr_t           off;

      if (!ptr_size || region->size < ptr_size)
        continue;

      for (off = 0; off < region->size; off += CL_POINTERSEARCH_CHUNK_SIZE)
      {
        cl_addr_t chunk_size = region->size - off;
        cl_addr_t k;

        if (chunk_size > CL_POINTERSEARCH_CHUNK_SIZE)
          chunk_size = CL_POINTERSEARCH_CHUNK_SIZE;

        if (cl_read_memory_buffer(chunk, region, off, chunk_size) != CL_OK)
          continue;

        search->memory_scanned += chunk_size;

        for (k = 0; k + ptr_size <= chunk_size; k += ptr_size)
        {
          cl_addr_t value = 0;
          cl_read_value(&value, chunk, k, ptr_type, region->endianness);

          if (value <= target_addr && value >= target_addr - search->range)
          {
            cl_pointersearch_result_t *result = &search->results[matches];
            result->address_initial = region->base_guest + off + k;
            result->address_final   = target_addr;
            result->offsets[0]      = target_addr - value;
            if (++matches >= search->max_results)
              goto first_pass_done;
          }
        }
      }
    }
    first_pass_done:
    search->result_count = matches;

    /* Subsequent passes: find pointers to each result's initial address */
    for (pass = 1; pass < search->passes; pass++)
      add_pass(search, pass, chunk);

    search->memory_usage = search->result_count * sizeof(cl_pointersearch_result_t);
    search->time_taken   = (double)(clock() - start) / CLOCKS_PER_SEC;
    search->results = realloc(search->results,
      search->result_count * sizeof(cl_pointersearch_result_t));

    /* Populate initial values so previous/current start in a valid state */
    cl_pointersearch_update(search);
    for (i = 0; i < search->result_count; i++)
      search->results[i].value_previous = search->results[i].value_current;

    free(chunk);
    return CL_OK;
  }
}

cl_error cl_pointersearch_update_result(cl_pointersearch_t *search,
  cl_pointersearch_result_t *result)
{
  cl_addr_t address = result->address_initial;
  unsigned  pass;

  for (pass = 0; pass < search->passes; pass++)
  {
    const cl_memory_region_t *region = cl_find_memory_region(address);

    if (!region)
      return CL_ERR_CLIENT_RUNTIME;
    else if (cl_read_memory_value(&address, NULL, address,
      cl_pointer_type(region->pointer_length)) != CL_OK)
      return CL_ERR_CLIENT_RUNTIME;

    address += result->offsets[pass];
  }

  result->address_final = address;
  cl_read_memory_value(result->value_current.raw, NULL, address,
    search->params.value_type);

  return CL_OK;
}

cl_error cl_pointersearch_update(cl_pointersearch_t *search)
{
  unsigned i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;

  for (i = 0; i < search->result_count; i++)
    cl_pointersearch_update_result(search, &search->results[i]);

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
    cl_pointersearch_target_impl_t *target_impl = CL_POINTERSEARCH_TARGET(search->params.target);

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
    cl_pointersearch_target_impl_t *target_impl = CL_POINTERSEARCH_TARGET(search->params.target);

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
    cl_pointersearch_target_impl_t *target_impl = CL_POINTERSEARCH_TARGET(search->params.target);

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
