#include "cl_search_pointer.h"

#include "cl_common.h"
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
   pointer-sized read from every non-ignored memory region whose value
   points into a memory region; nothing else can be a link in a chain, and
   on large address spaces most words are zeroes or plain data. It is built
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
 * Only its parents in executable image memory are kept, since a singleton
 * referenced all over the heap is often exactly what a static points to.
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

/* A span of guest memory that pointers can point into */
typedef struct
{
  cl_addr_t start;
  cl_addr_t end;
} cl_ptrmap_range_t;

static int ptrmap_range_cmp(const void *a, const void *b)
{
  const cl_ptrmap_range_t *ra = (const cl_ptrmap_range_t *)a;
  const cl_ptrmap_range_t *rb = (const cl_ptrmap_range_t *)b;
  if (ra->start < rb->start) return -1;
  if (ra->start > rb->start) return  1;
  return 0;
}

/**
 * Collects every memory region, ignored or not, as sorted and merged spans.
 * @return The number of spans written, which is at most memory.region_count
 */
static unsigned ptrmap_ranges(cl_ptrmap_range_t *ranges)
{
  unsigned count = 0;
  unsigned i;

  for (i = 0; i < memory.region_count; i++)
  {
    ranges[count].start = memory.regions[i].base_guest;
    ranges[count].end   = memory.regions[i].base_guest + memory.regions[i].size;
    count++;
  }
  if (!count)
    return 0;
  qsort(ranges, count, sizeof(*ranges), ptrmap_range_cmp);

  /* Merge touching or overlapping spans so each value has one candidate */
  for (i = 1, count = 1; i < memory.region_count; i++)
  {
    if (ranges[i].start <= ranges[count - 1].end)
    {
      if (ranges[i].end > ranges[count - 1].end)
        ranges[count - 1].end = ranges[i].end;
    }
    else
      ranges[count++] = ranges[i];
  }

  return count;
}

static cl_bool ptrmap_points_into(const cl_ptrmap_range_t *ranges,
  unsigned count, cl_addr_t value)
{
  unsigned lo = 0;
  unsigned hi = count;
  unsigned mid;

  if (!count || value < ranges[0].start || value >= ranges[count - 1].end)
    return CL_FALSE;

  /* Find the first span ending after the value */
  while (lo < hi)
  {
    mid = lo + (hi - lo) / 2;
    if (ranges[mid].end <= value)
      lo = mid + 1;
    else
      hi = mid;
  }

  return lo < count && value >= ranges[lo].start;
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
 * pointer-sized value that points into a memory region, and its guest
 * address, then sorts the result.
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
  uint8_t           *chunk;
  cl_ptrmap_range_t *ranges;
  unsigned           range_count;
  unsigned           i;
  cl_addr_t          scanned = 0;
  cl_error           error   = CL_OK;

  memset(map, 0, sizeof(*map));

  chunk  = (uint8_t *)malloc(CL_POINTERSEARCH_CHUNK_SIZE);
  ranges = (cl_ptrmap_range_t *)malloc(
    (memory.region_count ? memory.region_count : 1) * sizeof(*ranges));
  if (!chunk || !ranges)
  {
    free(chunk);
    free(ranges);
    return CL_ERR_PARAMETER_NULL;
  }
  range_count = ptrmap_ranges(ranges);

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
        if (!ptrmap_points_into(ranges, range_count, value))
          continue;
        error = ptrmap_push(map, value, region->base_guest + off + k);
        if (error != CL_OK)
          goto done;
      }
    }
  }

done:
  free(chunk);
  free(ranges);
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
   Backward search using the reverse pointer map
   ======================================================================

   The search walks backward from the target one pointer hop per pass. Every
   address that can reach the target is kept as a node in a tree, which only
   records the hop to its parent, so intermediate passes can hold millions of
   candidates cheaply. Only the final pass produces results, and only it is
   bound by max_results, since capping intermediate passes would drop the
   paths that lead back to a static root.
*/

/* A candidate address, which reaches its parent node by adding offset */
typedef struct
{
  cl_addr_t address;
  cl_addr_t offset;
  size_t    parent;
} cl_ptrsearch_node_t;

#define CL_PTRSEARCH_NO_PARENT ((size_t)-1)

/*
 * The most nodes kept across intermediate passes, to bound memory use when
 * the search space explodes. Hitting it means some chains may be missing.
 */
#define CL_PTRSEARCH_MAX_NODES (1 << 24)

/*
 * Whether a chain may start at the given address. Only a chain's initial
 * address, which is found on its final pass, is held to the base region.
 */
static cl_bool pointersearch_in_base(const cl_pointersearch_t *search,
  cl_addr_t address)
{
  return !search->base_size ||
         (address >= search->base_start &&
          address - search->base_start < search->base_size);
}

/**
 * Runs every pass of a new search over the reverse pointer map, writing up
 * to search->max_results chains into search->results.
 */
static cl_error ptrmap_search(cl_pointersearch_t *search, const cl_ptrmap_t *map)
{
  cl_ptrsearch_node_t *nodes;
  size_t    node_count = 1, node_capacity = 65536;
  size_t    level_lo = 0, level_hi = 1, q;
  cl_bool   truncated = CL_FALSE;
  unsigned  pass;

  search->result_count = 0;
  nodes = (cl_ptrsearch_node_t *)malloc(node_capacity * sizeof(*nodes));
  if (!nodes)
    return CL_ERR_PARAMETER_NULL;
  nodes[0].address = search->target_address;
  nodes[0].offset  = 0;
  nodes[0].parent  = CL_PTRSEARCH_NO_PARENT;

  for (pass = 0; pass < search->passes; pass++)
  {
    cl_bool last = pass + 1 == search->passes;

    for (q = level_lo; q < level_hi; q++)
    {
      cl_addr_t target = nodes[q].address;
      cl_addr_t lo_val = search->range <= target ? target - search->range : 0;
      cl_addr_t map_lo = ptrmap_lower_bound(map, lo_val);
      cl_addr_t map_hi = ptrmap_upper_bound(map, target);
      cl_addr_t per_target = 0;
      cl_addr_t idx;

      /* Pruning: a target with an excessive number of parents is likely a
       * widely-shared object. Expanding it through the heap would multiply
       * the candidates by thousands for no practical gain, so only parents
       * in static image memory are kept. */
      cl_bool prune = CL_PTRMAP_PRUNE_THRESHOLD > 0 &&
                      map_hi - map_lo > CL_PTRMAP_PRUNE_THRESHOLD;

      for (idx = map_lo; idx < map_hi; idx++)
      {
        cl_addr_t address = map->entries[idx].address;
        cl_addr_t offset  = target - map->entries[idx].value;

        if (last && !pointersearch_in_base(search, address))
          continue;
        if (prune)
        {
          const cl_memory_region_t *region = cl_find_memory_region(address);

          if (!region || !region->flags.bits.image)
            continue;
        }

        if (last)
        {
          cl_pointersearch_result_t *out = &search->results[search->result_count++];
          size_t   n = q;
          unsigned k = 1;

          /* Offsets run outward from the initial address to the target */
          out->address_initial = address;
          out->address_final   = search->target_address;
          out->offsets[0]      = offset;
          while (nodes[n].parent != CL_PTRSEARCH_NO_PARENT && k < CL_POINTER_MAX_PASSES)
          {
            out->offsets[k++] = nodes[n].offset;
            n = nodes[n].parent;
          }

          if (search->result_count >= search->max_results)
            goto done;
        }
        else
        {
          if (node_count >= CL_PTRSEARCH_MAX_NODES)
          {
            truncated = CL_TRUE;
            break;
          }
          if (node_count >= node_capacity)
          {
            cl_ptrsearch_node_t *p = (cl_ptrsearch_node_t *)realloc(nodes,
              node_capacity * 2 * sizeof(*nodes));

            if (!p)
            {
              truncated = CL_TRUE;
              break;
            }
            nodes = p;
            node_capacity *= 2;
          }
          nodes[node_count].address = address;
          nodes[node_count].offset  = offset;
          nodes[node_count].parent  = q;
          node_count++;
        }

        /* Stop expanding this target once the per-pass cap is reached */
        if (search->max_results_per_pass &&
            ++per_target >= search->max_results_per_pass)
          break;
      }
    }
    level_lo = level_hi;
    level_hi = node_count;
  }

done:
  free(nodes);
  if (truncated)
    cl_message(CL_MSG_WARN, "Pointer search hit its candidate limit, so "
      "some chains may be missing. Lower the offset range or "
      "pointer levels to search more thoroughly.");

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
  search->base_start           = 0;
  search->base_size            = 0;
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

    error = ptrmap_search(search, &map);
    if (error != CL_OK)
    {
      ptrmap_free(&map);
      free(search->results);
      search->results = NULL;
      return error;
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

cl_error cl_pointersearch_change_base_region(cl_pointersearch_t *search,
  const cl_memory_region_t *region)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (search->results)
    return CL_ERR_CLIENT_RUNTIME;
  else
  {
    search->base_start = region ? region->base_guest : 0;
    search->base_size  = region ? region->size : 0;

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
