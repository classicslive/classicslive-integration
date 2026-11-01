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

/* ======================================================================
   Reverse pointer map
   ======================================================================

   The map is a flat array of (value, address) pairs covering every
   pointer-sized read from every non-ignored memory region. It is built
   once at the start of a pointer search and sorted by value, enabling
   O(log n) range queries for each subsequent pass instead of O(memory).

   For large address spaces (hundreds of MB to GBs) the sort is a
   one-time O(n log n) cost; each pass then costs O(log n + results),
   vs the old O(passes * memory_size) per search.
*/

typedef struct
{
  cl_addr_t value;   /* pointer value stored at .address           */
  cl_addr_t address; /* guest address where this pointer was found */
} cl_ptrmap_entry_t;

typedef struct
{
  cl_ptrmap_entry_t *entries;
  cl_addr_t          count;
  cl_addr_t          capacity;
} cl_ptrmap_t;

/*
 * When a single address has more than this many parents in the map, it is
 * likely a widely-shared object (e.g. a vtable or common heap node) and
 * expanding it would cause a combinatorial explosion in the result set.
 * Such targets are skipped during multi-pass expansion.
 *
 * Set to 0 to disable pruning.
 */
#define CL_PTRMAP_PRUNE_THRESHOLD 256

static int ptrmap_entry_cmp(const void *a, const void *b)
{
  const cl_ptrmap_entry_t *ea = (const cl_ptrmap_entry_t *)a;
  const cl_ptrmap_entry_t *eb = (const cl_ptrmap_entry_t *)b;
  if (ea->value < eb->value) return -1;
  if (ea->value > eb->value) return  1;
  return 0;
}

/* Index of first entry with value >= target (lower_bound). */
static cl_addr_t ptrmap_lower_bound(const cl_ptrmap_t *map, cl_addr_t target)
{
  cl_addr_t lo = 0;
  cl_addr_t hi = map->count;
  cl_addr_t mid;

  while (lo < hi)
  {
    mid = lo + (hi - lo) / 2;
    if (map->entries[mid].value < target)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

/* Index of first entry with value > target (upper_bound). */
static cl_addr_t ptrmap_upper_bound(const cl_ptrmap_t *map, cl_addr_t target)
{
  cl_addr_t lo = 0;
  cl_addr_t hi = map->count;
  cl_addr_t mid;

  while (lo < hi)
  {
    mid = lo + (hi - lo) / 2;
    if (map->entries[mid].value <= target)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

static cl_error ptrmap_push(cl_ptrmap_t *map, cl_addr_t value, cl_addr_t address)
{
  cl_addr_t          new_cap;
  cl_ptrmap_entry_t *p;

  if (map->count >= map->capacity)
  {
    new_cap = map->capacity ? map->capacity * 2 : 65536;
    if (new_cap < map->capacity)      /* overflow guard */
      return CL_ERR_CLIENT_RUNTIME;
    p = (cl_ptrmap_entry_t *)realloc(
      map->entries, (size_t)new_cap * sizeof(cl_ptrmap_entry_t));
    if (!p)
      return CL_ERR_PARAMETER_NULL;
    map->entries  = p;
    map->capacity = new_cap;
  }
  map->entries[map->count].value   = value;
  map->entries[map->count].address = address;
  map->count++;
  return CL_OK;
}

static void ptrmap_free(cl_ptrmap_t *map)
{
  free(map->entries);
  map->entries  = NULL;
  map->count    = 0;
  map->capacity = 0;
}

/**
 * Scans all non-ignored memory regions once and records every
 * pointer-sized value and its guest address, then sorts the result.
 *
 * This is the only full-memory scan for an entire multi-pass search.
 * Memory is read in CL_POINTERSEARCH_CHUNK_SIZE buckets to amortise
 * per-read overhead.  The sort is O(n log n) where n is the total
 * number of pointer-sized words across all regions.
 *
 * Further optimisation: regions with uniform pointer size and endianness
 * could be scanned with SIMD byte-swap intrinsics, but that would
 * require platform-specific code paths.
 */
static cl_error ptrmap_build(cl_ptrmap_t *map, cl_addr_t *scanned_out)
{
  uint8_t  *chunk;
  unsigned  i;
  cl_addr_t scanned = 0;
  cl_error  error   = CL_OK;

  memset(map, 0, sizeof(*map));

  chunk = (uint8_t *)malloc(CL_POINTERSEARCH_CHUNK_SIZE);
  if (!chunk)
    return CL_ERR_PARAMETER_NULL;

  for (i = 0; i < memory.region_count; i++)
  {
    cl_memory_region_t *region   = &memory.regions[i];
    cl_value_type       ptr_type = cl_pointer_type(region->pointer_length);
    cl_addr_t           ptr_size = region->pointer_length;
    cl_addr_t           off;

#if CL_HAVE_EDITOR
    if (region->ignore)
      continue;
#endif
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

      scanned += chunk_size;

      for (k = 0; k + ptr_size <= chunk_size; k += ptr_size)
      {
        cl_addr_t value = 0;
        cl_read_value(&value, chunk, k, ptr_type, region->endianness);
        error = ptrmap_push(map, value, region->base_guest + off + k);
        if (error != CL_OK)
          goto done;
      }
    }
  }

done:
  free(chunk);
  if (error == CL_OK)
  {
    /* Sort by value so all subsequent passes can use binary search */
    qsort(map->entries, (size_t)map->count,
          sizeof(cl_ptrmap_entry_t), ptrmap_entry_cmp);
    if (scanned_out)
      *scanned_out = scanned;
  }
  else
    ptrmap_free(map);

  return error;
}

/* ======================================================================
   Pass execution using the reverse pointer map
   ====================================================================== */

/**
 * Pass 0: find all map entries whose stored value falls within
 * [target - range, target].  Each hit is an address that holds a pointer
 * pointing directly into the target's address range.
 *
 * Result layout after pass 0:
 *   address_initial  = address containing the pointer
 *   offsets[0]       = target - stored_value  (how far past the pointer)
 *   address_final    = target
 */
static cl_addr_t ptrmap_pass0(const cl_ptrmap_t *map,
  cl_addr_t target, cl_addr_t range,
  cl_pointersearch_result_t *out, cl_addr_t max_out,
  cl_addr_t max_per_pass)
{
  cl_addr_t lo_val = (range <= target) ? (target - range) : 0;
  cl_addr_t lo     = ptrmap_lower_bound(map, lo_val);
  cl_addr_t hi     = ptrmap_upper_bound(map, target);
  cl_addr_t idx;
  cl_addr_t count  = 0;

  for (idx = lo; idx < hi; idx++)
  {
    if (count >= max_out)
      break;
    if (max_per_pass && count >= max_per_pass)
      break;

    out[count].address_initial = map->entries[idx].address;
    out[count].address_final   = target;
    out[count].offsets[0]      = target - map->entries[idx].value;
    count++;
  }
  return count;
}

/*
 * Comparison used to sort results by address_initial before multi-pass
 * expansion so that identical targets can be grouped and processed once.
 */
static int result_cmp_initial(const void *a, const void *b)
{
  const cl_pointersearch_result_t *ra = (const cl_pointersearch_result_t *)a;
  const cl_pointersearch_result_t *rb = (const cl_pointersearch_result_t *)b;
  if (ra->address_initial < rb->address_initial) return -1;
  if (ra->address_initial > rb->address_initial) return  1;
  return 0;
}

/**
 * Subsequent passes: expand the current result set by walking backward
 * one more pointer hop using the reverse map.
 *
 * Algorithm for each unique address_initial T in results:
 *   1. Binary-search ptrmap for entries with value in [T - range, T].
 *   2. For each such map entry (new_addr, val):
 *        For each existing result whose address_initial == T:
 *          Create a new result:
 *            new.address_initial = new_addr
 *            new.offsets[0]      = T - val        (offset for this new hop)
 *            new.offsets[1..]    = old.offsets[..] (existing chain, shifted up)
 *
 * Sorting the results before this function ensures each unique target T
 * is looked up exactly once, regardless of how many existing chains
 * share it as their head — avoiding redundant binary searches.
 *
 * Pruning: targets with more than CL_PTRMAP_PRUNE_THRESHOLD parents are
 * skipped to prevent combinatorial explosion from widely-shared objects
 * (e.g. common vtables, reference-counted heap nodes).
 *
 * @param pass  Pass index, 1-based (1 = second expansion, 2 = third, ...)
 *              Used to know how many offsets to shift from the old chain.
 */
static cl_error ptrmap_expand_pass(cl_pointersearch_t *search,
  const cl_ptrmap_t *map, unsigned pass)
{
  cl_pointersearch_result_t *new_results;
  cl_addr_t new_count;
  cl_addr_t i;
  /* Declare all loop-body locals here for C89 compatibility */
  cl_addr_t current_target;
  cl_addr_t results_lo, results_hi;
  cl_addr_t map_lo, map_hi, map_idx;
  cl_addr_t r, k;
  cl_addr_t lo_val;
  cl_addr_t per_target;
  cl_addr_t new_addr, new_offset;

  new_results = (cl_pointersearch_result_t *)calloc(
    search->max_results, sizeof(cl_pointersearch_result_t));
  if (!new_results)
    return CL_ERR_PARAMETER_NULL;

  new_count = 0;

  /* Sort by address_initial so each unique target is grouped contiguously.
   * This is the key deduplication step: instead of scanning memory once per
   * result (old behaviour), we now do one binary search per unique target. */
  qsort(search->results, (size_t)search->result_count,
        sizeof(*search->results), result_cmp_initial);

  i = 0;
  while (i < search->result_count && new_count < search->max_results)
  {
    per_target     = 0;
    current_target = search->results[i].address_initial;
    results_lo     = i;

    /* Find the end of the group sharing current_target */
    while (i < search->result_count &&
           search->results[i].address_initial == current_target)
      i++;
    results_hi = i;

    /* Look up all stored pointers within [current_target - range, current_target] */
    lo_val  = (search->range <= current_target)
              ? (current_target - search->range) : 0;
    map_lo  = ptrmap_lower_bound(map, lo_val);
    map_hi  = ptrmap_upper_bound(map, current_target);

    /* Pruning: a target with an excessive number of parents is likely a
     * widely-shared object.  Expanding it would multiply the result count
     * by thousands for no practical gain.  Skip it entirely; other branches
     * in the result set will still find valid chains. */
    if (CL_PTRMAP_PRUNE_THRESHOLD > 0 &&
        map_hi - map_lo > CL_PTRMAP_PRUNE_THRESHOLD)
      continue;

    for (map_idx = map_lo;
         map_idx < map_hi && new_count < search->max_results;
         map_idx++)
    {
      new_addr   = map->entries[map_idx].address;
      new_offset = current_target - map->entries[map_idx].value;

      /* Fan out: one new result per existing chain that passes through
       * current_target.  In the common case (results_lo + 1 == results_hi)
       * this inner loop executes exactly once. */
      for (r = results_lo;
           r < results_hi && new_count < search->max_results;
           r++)
      {
        cl_pointersearch_result_t *old = &search->results[r];
        cl_pointersearch_result_t *out = &new_results[new_count];

        out->address_initial = new_addr;
        out->address_final   = search->target_address;

        /* Insert the new hop at offsets[0] and shift the existing chain up.
         * Example for pass=2 (third pointer level):
         *   out->offsets[0] = new_offset          (new hop: new_addr → current_target)
         *   out->offsets[1] = old->offsets[0]     (old hop: current_target → ...)
         *   out->offsets[2] = old->offsets[1]     (old hop: ... → target_address)  */
        out->offsets[0] = new_offset;
        for (k = 0; k < (cl_addr_t)pass && k + 1 < CL_POINTER_MAX_PASSES; k++)
          out->offsets[k + 1] = old->offsets[k];

        new_count++;
        per_target++;

        /* Stop expanding this target once the per-pass cap is reached */
        if (search->max_results_per_pass &&
            per_target >= search->max_results_per_pass)
          goto next_target;
      }
    }
    next_target: ;
  }

  free(search->results);
  search->results      = new_results;
  search->result_count = new_count;
  return CL_OK;
}

/* ======================================================================
   Public API
   ====================================================================== */

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
  search->params.value_type    = value_type;
  search->params.value_size    = cl_sizeof_memtype(value_type);
  search->params.compare_type  = CL_COMPARE_EQUAL;
  search->params.target_none   = 1;

  search->params.target_ptr = CL_POINTERSEARCH_TARGET(search->params.target);

  return CL_OK;
}

cl_error cl_pointersearch_step(cl_pointersearch_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;

  /* ----------------------------------------------------------------
     Subsequent steps: re-resolve each chain and filter by the current
     value at the final address.  The pointer map is not needed here;
     we just dereference in real time.
   ---------------------------------------------------------------- */
  if (search->results)
  {
    clock_t   start   = clock();
    cl_addr_t matches = 0;
    unsigned  i;

    for (i = 0; i < search->result_count; i++)
    {
      cl_pointersearch_result_t *result = &search->results[i];
      cl_addr_t address = result->address_initial;
      unsigned  pass;
      cl_bool   failed  = CL_FALSE;

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

    search->result_count   = matches;
    search->memory_scanned = 0;
    search->memory_usage   = matches * sizeof(cl_pointersearch_result_t);
    search->time_taken     = (double)(clock() - start) / CLOCKS_PER_SEC;
    search->results = realloc(search->results,
      matches * sizeof(cl_pointersearch_result_t));
    return CL_OK;
  }

  /* ----------------------------------------------------------------
     First step: build the reverse pointer map, then use it for all
     passes without any further full-memory scans.

     Old complexity: O(passes * result_count * memory_size)
     New complexity: O(memory_size * log(memory_size))  [map build + sort]
                   + O(unique_targets * log(map_size))  [per-pass lookups]

     The map is freed immediately after the search completes; it is not
     needed for the value-monitoring steps above.
   ---------------------------------------------------------------- */
  {
    cl_ptrmap_t map;
    cl_addr_t   scanned = 0;
    clock_t     start   = clock();
    unsigned    pass;
    cl_error    error;

    search->results = (cl_pointersearch_result_t *)calloc(
      search->max_results, sizeof(cl_pointersearch_result_t));
    if (!search->results)
      return CL_ERR_PARAMETER_NULL;

    /* Single full scan of all non-ignored regions to build the map */
    error = ptrmap_build(&map, &scanned);
    if (error != CL_OK)
    {
      free(search->results);
      search->results = NULL;
      return error;
    }
    search->memory_scanned = scanned;

    /* Pass 0: direct pointers into target_address ± range */
    search->result_count = ptrmap_pass0(&map,
      search->target_address, search->range,
      search->results, search->max_results,
      search->max_results_per_pass);

    /* Passes 1..N-1: expand backward one hop per pass using the map.
     * Each pass replaces search->results with a new array where every
     * result's address_initial has been pushed one level back.         */
    for (pass = 1; pass < search->passes; pass++)
    {
      error = ptrmap_expand_pass(search, &map, pass);
      if (error != CL_OK)
      {
        ptrmap_free(&map);
        return error;
      }
    }

    ptrmap_free(&map);

    search->memory_usage = search->result_count * sizeof(cl_pointersearch_result_t);
    search->time_taken   = (double)(clock() - start) / CLOCKS_PER_SEC;
    search->results = realloc(search->results,
      search->result_count * sizeof(cl_pointersearch_result_t));

    /* Snapshot current values so previous/current start in a valid state */
    cl_pointersearch_update(search);
    {
      unsigned i;
      for (i = 0; i < search->result_count; i++)
        search->results[i].value_previous = search->results[i].value_current;
    }

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
