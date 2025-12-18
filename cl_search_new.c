#include "cl_search_new.h"

#include "cl_config.h"
#include "cl_memory.h"

#if CL_HOST_PLATFORM == CL_PLATFORM_LINUX
  #include <sys/mman.h>
#elif CL_HOST_PLATFORM == CL_PLATFORM_WINDOWS
  #include <windows.h>
#endif

#include <string.h>

#if CL_EXTERNAL_MEMORY
#ifndef CL_SEARCH_BUCKET_SIZE
/**
 * The amount of data to retrieve from the external process at a time when
 * processing a search.
 */
#define CL_SEARCH_BUCKET_SIZE CL_MB(128)
#endif
#endif

#ifndef CL_SEARCH_CHUNK_SIZE
/**
 * The granularity of data to keep in memory as search results.
 * The total allocation per chunk is twice this size, as the validity bitmap
 * is stored alongside the data itself.
 * This value was decided on by guessing to see which was most performant. :B
 * @todo Make configurable?
 */
#define CL_SEARCH_CHUNK_SIZE CL_MB(4)
#endif

/**
 * Allocate a chunk of page-aligned memory.
 * @param size The number of bytes to allocate
 * @return A pointer to the bytes, or NULL
 */
static void *cl_mmap(size_t size)
{
#if CL_HOST_PLATFORM == CL_PLATFORM_LINUX
  void *p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (p == CL_ADDRESS_INVALID)
    return NULL;
  else
    return p;
#elif CL_HOST_PLATFORM == CL_PLATFORM_WINDOWS
  return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  return malloc(size);
#endif
}

/**
 * Free a chunk of page-aligned memory.
 * @param p The pointer to free
 * @param size The number of bytes to deallocate
 */
static void cl_munmap(void *p, size_t size)
{
#if CL_HOST_PLATFORM == CL_PLATFORM_LINUX
  munmap(p, size);
#elif CL_HOST_PLATFORM == CL_PLATFORM_WINDOWS
  CL_UNUSED(size);
  VirtualFree(p, 0, MEM_RELEASE);
#else
  CL_UNUSED(size);
  free(p);
#endif
}

#define CL_PASTE2(a, b) a##b
#define CL_PASTE3(a, b, c) a##b##c

#define CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE3(cl_search_cmp_imm_, b, _##d)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a right = ((cl_search_target_t*)target)->b; \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (*chunk_data_cast c right) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
  } \
  return matches; \
}

#define CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE3(cl_search_cmp_prv_, b, _##d)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a *chunk_data_prev_cast = (const a*)chunk_data_prev; \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (*chunk_data_cast c *chunk_data_prev_cast) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
    chunk_data_prev_cast++; \
  } \
  return matches; \
}

#define CL_SEARCH_CMP_IMMEDIATE_UNROLL(a, b) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, !=, neq) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, !=, neq) \

CL_SEARCH_CMP_IMMEDIATE_UNROLL(unsigned char, u8)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(signed char, s8)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(unsigned short, u16)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(signed short, s16)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(unsigned int, u32)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(signed int, s32)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(signed long, s64)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(float, fp)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(double, dfp)

typedef unsigned (*cl_search_compare_func_t)(void*,const void*,unsigned char*,const void*);

static cl_search_compare_func_t cl_search_comparison_function(cl_search_parameters_t params)
{
  switch (params.value_type)
  {
  case CL_MEMTYPE_UINT8:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u8_equ : cl_search_cmp_imm_u8_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_u8_gtr : cl_search_cmp_imm_u8_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_u8_les : cl_search_cmp_imm_u8_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u8_neq : cl_search_cmp_imm_u8_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_INT8:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s8_equ : cl_search_cmp_imm_s8_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_s8_gtr : cl_search_cmp_imm_s8_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_s8_les : cl_search_cmp_imm_s8_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s8_neq : cl_search_cmp_imm_s8_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_UINT16:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u16_equ : cl_search_cmp_imm_u16_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_u16_gtr : cl_search_cmp_imm_u16_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_u16_les : cl_search_cmp_imm_u16_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u16_neq : cl_search_cmp_imm_u16_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_INT16:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s16_equ : cl_search_cmp_imm_s16_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_s16_gtr : cl_search_cmp_imm_s16_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_s16_les : cl_search_cmp_imm_s16_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s16_neq : cl_search_cmp_imm_s16_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_UINT32:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u32_equ : cl_search_cmp_imm_u32_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_u32_gtr : cl_search_cmp_imm_u32_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_u32_les : cl_search_cmp_imm_u32_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_u32_neq : cl_search_cmp_imm_u32_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_INT32:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s32_equ : cl_search_cmp_imm_s32_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_s32_gtr : cl_search_cmp_imm_s32_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_s32_les : cl_search_cmp_imm_s32_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s32_neq : cl_search_cmp_imm_s32_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_INT64:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s64_equ : cl_search_cmp_imm_s64_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_s64_gtr : cl_search_cmp_imm_s64_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_s64_les : cl_search_cmp_imm_s64_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_s64_neq : cl_search_cmp_imm_s64_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_DOUBLE:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_dfp_equ : cl_search_cmp_imm_dfp_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_dfp_gtr : cl_search_cmp_imm_dfp_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_dfp_les : cl_search_cmp_imm_dfp_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_dfp_neq : cl_search_cmp_imm_dfp_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_FLOAT:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_fp_equ : cl_search_cmp_imm_fp_equ;
    case CL_COMPARE_GREATER:
      return params.compare_to_previous ? cl_search_cmp_prv_fp_gtr : cl_search_cmp_imm_fp_gtr;
    case CL_COMPARE_LESS:
      return params.compare_to_previous ? cl_search_cmp_prv_fp_les : cl_search_cmp_imm_fp_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.compare_to_previous ? cl_search_cmp_prv_fp_neq : cl_search_cmp_imm_fp_neq;
    default:
      return NULL;
    }
  case CL_MEMTYPE_NOT_SET:
  case CL_MEMTYPE_SIZE:
    /* No default because we want a warning if we add new types */
    return NULL;
  }

  return NULL;
}

/**
 * Runs a comparison function on the values in a search page.
 * @param page
 */
static cl_error cl_search_step_page(cl_search_page_t *page,
  const cl_search_parameters_t params, cl_search_compare_func_t function,
  const void *prev_buffer)
{
  const void *end = (((unsigned char*)page->chunk) + page->size);

  if (!function)
    return CL_ERR_PARAMETER_INVALID;
  page->matches = function(page->chunk, end, page->validity,
    params.compare_to_previous ? prev_buffer : &params.target);

  return CL_OK;
}

/**
 * Steps through the linked list to count up the total memory usage of a
 * search, storing the result in `search->memory_usage` as bytes.
 */
static cl_error cl_search_profile_memory(cl_search_t *search)
{
  cl_addr_t usage = 0;
  unsigned i;

  for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_t *page = search->page_regions[i].first_page;

    while (page)
    {
      /* Count the chunks */
      usage += page->size + page->size / search->params.value_size;
      usage += sizeof(cl_search_page_t);
      page = page->next;
    }
    usage += sizeof(cl_search_page_region_t);
  }
  usage += sizeof(cl_search_t);
  search->memory_usage = usage;

  return CL_OK;
}

cl_error cl_search_change_compare_type(cl_search_t *search,
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

cl_error cl_search_change_value_type(cl_search_t *search, cl_value_type type)
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

cl_error cl_search_change_target(cl_search_t *search, const void *value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (!value)
    search->params.compare_to_previous = 1;
  else
  {
    cl_search_target_t target;

    memset(&target, 0, sizeof(cl_search_target_t));
    switch (search->params.value_size)
    {
    case 1:
      memcpy(&target.u8, value, 1);
      break;
    case 2:
      memcpy(&target.u16, value, 2);
      break;
    case 4:
      memcpy(&target.u32, value, 4);
      break;
    case 8:
      memcpy(&target.u64, value, 8);
      break;
    default:
      return CL_ERR_PARAMETER_INVALID;
    }
    search->params.target = target;
    search->params.compare_to_previous = 0;
  }

  return CL_OK;
}

static cl_error cl_search_free_page(const cl_search_t *search,
  cl_search_page_t *page)
{
  if (page)
  {
    cl_munmap(page->chunk, page->size + page->size / search->params.value_size);
    free(page);

    return CL_OK;
  }

  return CL_ERR_PARAMETER_NULL;
}

cl_error cl_search_free(cl_search_t *search)
{
  unsigned i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];
    cl_search_page_t *page = page_region->first_page;
    cl_search_page_t *next_page = NULL;

    while (page)
    {
      next_page = page->next;
      cl_search_free_page(search, page);
      page = next_page;
    }
  }
  free(search->page_regions);
  memset(search, 0, sizeof(cl_search_t));

  return CL_OK;
}

cl_error cl_search_init(cl_search_t *search)
{
  unsigned i;

  if (!search || !memory.regions)
    return CL_ERR_PARAMETER_NULL;
  else if (memory.region_count == 0)
    return CL_ERR_PARAMETER_INVALID;

  /* Zero-init the search */
  memset(search, 0, sizeof(cl_search_t));

  /* Allocate and init page regions */
  search->page_regions = (cl_search_page_region_t*)calloc(
    memory.region_count, sizeof(cl_search_page_region_t));
  search->page_region_count = memory.region_count;
  for (i = 0; i < memory.region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];

    page_region->page_count = 0;
    page_region->region = &memory.regions[i];
  }

  return cl_search_profile_memory(search);
}

/**
 * Performs the first search step, which is responsible for allocating the
 * initial round of chunks.
 * @param search A pointer to the search to perform a step on
 */
static cl_error cl_search_step_first(cl_search_t *search)
{
#if CL_EXTERNAL_MEMORY
  void *bucket = NULL;
  cl_addr_t bucket_offset = 0;
  cl_addr_t bucket_processed = 0;
  cl_addr_t bucket_size = 0;
#endif
  cl_search_compare_func_t function = cl_search_comparison_function(search->params);
  unsigned i;

  if (!function)
    return CL_ERR_PARAMETER_INVALID;

#if CL_EXTERNAL_MEMORY
  bucket = cl_mmap(CL_SEARCH_BUCKET_SIZE);
  if (!bucket)
    return CL_ERR_CLIENT_RUNTIME;
#endif

  for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];
    cl_search_page_t *page = NULL;
    cl_search_page_t *prev_page = NULL;
    cl_addr_t processed = 0;

#if CL_EXTERNAL_MEMORY
    bucket_offset = 0;
    bucket_processed = 0;
    bucket_size = page_region->region->size < CL_SEARCH_BUCKET_SIZE ?
                  page_region->region->size : CL_SEARCH_BUCKET_SIZE;
    cl_read_memory_buffer(bucket, page_region->region, bucket_offset, bucket_size);
#endif

    while (processed < page_region->region->size)
    {
      unsigned size = CL_SEARCH_CHUNK_SIZE;

      if (processed + CL_SEARCH_CHUNK_SIZE > page_region->region->size)
        size = (unsigned)(page_region->region->size - processed);

      if (!page)
      {
        page = (cl_search_page_t*)calloc(1, sizeof(cl_search_page_t));
        page->chunk = cl_mmap(size + size / search->params.value_size);
        page->validity = (void*)((unsigned char*)page->chunk + size);
      }

      page->region = page_region->region;
      page->start = page_region->region->base_guest + processed;
      page->size = size;

#if CL_EXTERNAL_MEMORY
      memcpy(page->chunk, (unsigned char*)bucket + bucket_processed, page->size);
#else
      cl_read_memory_buffer(page->chunk, page->region,
        page->start - page->region->base_guest, size);
#endif

      memset(page->validity, 1, size / search->params.value_size);

      cl_search_step_page(page, search->params, function, NULL);

      if (page->matches == 0)
      {
        /* reuse this page for next chunk */
      }
      else
      {
        if (!prev_page)
          page_region->first_page = page;
        else
          prev_page->next = page;

        prev_page = page;
        page_region->matches += page->matches;
        search->total_matches += page->matches;
        page_region->page_count++;
        search->total_page_count++;

        page = NULL;
      }

      processed += size;

#if CL_EXTERNAL_MEMORY
      bucket_processed += size;
      if (bucket_processed >= bucket_size)
      {
        bucket_offset += bucket_size;
        if (bucket_offset < page_region->region->size)
        {
          bucket_size = page_region->region->size - bucket_offset;
          if (bucket_size > CL_SEARCH_BUCKET_SIZE)
            bucket_size = CL_SEARCH_BUCKET_SIZE;

          cl_read_memory_buffer(bucket, page_region->region, bucket_offset, bucket_size);
        }
        bucket_processed = 0;
      }
#endif
    }

    if (page && page->matches == 0)
      cl_search_free_page(search, page);
  }

  search->steps = 1;

#if CL_EXTERNAL_MEMORY
  cl_munmap(bucket, CL_SEARCH_BUCKET_SIZE);
#endif

  return cl_search_profile_memory(search);
}

cl_error cl_search_step(cl_search_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (search->steps == 0)
    return cl_search_step_first(search);
  else
  {
    cl_search_compare_func_t function;
    cl_addr_t total_matches = 0;
    unsigned i;

    function = cl_search_comparison_function(search->params);
    if (!function)
      return CL_ERR_PARAMETER_INVALID;

    for (i = 0; i < search->page_region_count; i++)
    {
      cl_search_page_region_t *page_region = &search->page_regions[i];
      cl_search_page_t *page = page_region->first_page;
      cl_search_page_t *prev_page = NULL;
      cl_search_page_t *next_page = NULL;
      cl_addr_t page_region_matches = 0;

      while (page)
      {
        cl_error error;
        
        cl_read_memory_buffer(page->chunk, page->region,
          page->start - page->region->base_guest, page->size);
        error = cl_search_step_page(page, search->params, function, NULL);

        if (error != CL_OK)
          return error;
        else if (page->matches == 0)
        {
          /* Remove this page from the linked list */
          if (prev_page)
            prev_page->next = page->next;
          else
            page_region->first_page = page->next;

          next_page = page->next;
          cl_search_free_page(search, page);
          page_region->page_count--;
          search->total_page_count--;
          page = next_page;
        }
        else
        {
          /* Keep this page in the linked list */
          if (!prev_page)
            page_region->first_page = page;
          prev_page = page;
          page_region_matches += page->matches;
          page = page->next;
        }
      }
      if (prev_page)
        prev_page->next = NULL;
      else
        page_region->first_page = NULL;
      page_region->matches = page_region_matches;
      total_matches += page_region_matches;
    }
    search->total_matches = total_matches;
    search->steps++;

    return cl_search_profile_memory(search);
  }
}

cl_error cl_search_backup_value(void *dst, const cl_search_t *search,
  cl_addr_t address)
{
  cl_search_page_region_t *page_region;
  cl_search_page_t *page;
  unsigned i;

#if 0 /* this is already too slow */
  if (!search || !dst)
    return CL_ERR_PARAMETER_NULL;
#endif

  for (i = 0; i < search->page_region_count; i++)
  {
    page_region = &search->page_regions[i];

    /* Is it in this region? */
    if (address < page_region->region->base_guest ||
        address >= page_region->region->base_guest + page_region->region->size)
      continue;

    page = page_region->first_page;

    while (page)
    {
      /* Is it in this page? */
      if (address >= page->start && address < page->start + page->size)
      {
        cl_addr_t offset = address - page->start;
        memcpy(dst, (unsigned char*)page->chunk + offset, search->params.value_size);
        return CL_OK;
      }
      page = page->next;
    }
  }

  return CL_ERR_PARAMETER_INVALID;
}
