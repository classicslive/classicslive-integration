#ifndef CL_SEARCH_H
#define CL_SEARCH_H

#include "cl_types.h"

typedef struct cl_search_page_t cl_search_page_t;

struct cl_search_page_t
{
  /* Number of matches found in this page */
  cl_addr_t matches;

  /* A buffer containing the memory chunk data and validity bitmap */
  void *chunk;

  /* A pointer to the position of the validity bitmap within the chunk */
  void *validity;

  /* The starting address of the memory chunk */
  cl_addr_t start;

  /* The first address of the memory chunk that resulted in a match */
  cl_addr_t first;

  /* The last address of the memory chunk that resulted in a match */
  cl_addr_t end;

  /**
   * The size, in bytes, of the data in the memory chunk. Should typically be
   * `CL_SEARCH_CHUNK_SIZE` except if the target region has memory smaller
   * than that or if it's the last chunk in a region that can't be divided
   * equally.
   * The chunk size should be `size + size / search->params.value_size`.
   */
  cl_addr_t size;

  /* Next page in the linked list */
  cl_search_page_t *next;

  /* The memory region this page is within */
  const cl_memory_region_t *region;
};

/* A wrapper for search pages to group them by memory region */
typedef struct
{
  /* The memory region these pages fall under */
  const cl_memory_region_t *region;

  /* The first page in the linked list */
  cl_search_page_t *first_page;

  /* The number of pages in this page region */
  unsigned page_count;

  /* The total number of matches in this page region */
  cl_addr_t matches;
} cl_search_page_region_t;

typedef union
{
  unsigned char u8;
  signed char s8;
  unsigned short u16;
  signed short s16;
  unsigned int u32;
  signed int s32;
  unsigned long u64;
  signed long s64;
  float fp;
  double dfp;
} cl_search_target_t;

typedef struct
{
  /* The type of value being searched for */
  cl_value_type value_type;

  /**
   * The size, in bytes, of the value type.
   * Should only ever be 1, 2, 4, or 8.
   */
  unsigned value_size;

  /* The comparison method to use */
  cl_compare_type compare_type;

  /* The target value to compare against */
  cl_search_target_t target;

  /**
   * A pointer to the target value to compare against. This offsets to the
   * correct position of the target union.
   */
  const void *target_ptr;

  /* Whether to compare to the previous value instead of a target */
  unsigned compare_to_previous;
} cl_search_parameters_t;

/** 
 * The main structure representing an ongoing memory search.
 * Upon creating a search, call `cl_search_init` to initialize it.
 * To free the search, call `cl_search_free`.
 */
typedef struct
{
  /**
   * The parameters used for this search. Edit this by using
   * `cl_search_change_params`.
   */
  cl_search_parameters_t params;

  cl_search_page_region_t *page_regions;
  unsigned page_region_count;

  /* The number of times `cl_search_step` has been called with this search */
  unsigned steps;

  /* The total number of pages allocated for this search */
  unsigned total_page_count;

  /* The total number of matches found in the search */
  cl_addr_t total_matches;

  /* The total memory usage of the search, in bytes */
  cl_addr_t memory_usage;
} cl_search_t;

/**
 * Changes the comparison type used in the search.
 * @param search A pointer to the search to modify
 * @param compare_type The new comparison type to use
 */
cl_error cl_search_change_compare_type(cl_search_t *search,
  cl_compare_type compare_type);

/**
 * Changes the value type used in the search. This cannot be used once the
 * search has begun (i.e. after calling `cl_search_step`).
 * @param search A pointer to the search to modify
 * @param type The new value type to use
 */
cl_error cl_search_change_value_type(cl_search_t *search, cl_value_type type);

/**
 * Changes the target value used in the search.
 * @param search A pointer to the search to modify
 * @param value A pointer to the new target value to use, or NULL to compare
 *   against the previous value instead
 */
cl_error cl_search_change_target(cl_search_t *search, const void *value);

cl_error cl_search_free(cl_search_t *search);

/**
 * Initializes a memory search structure.
 * @param search A pointer to the search to initialize
 */
cl_error cl_search_init(cl_search_t *search);

/**
 * Filters the values to only those that match the given `params`. The total
 * number of matches will then be available in `total_matches`.
 * @param search A pointer to the search to perform a step on
 */
cl_error cl_search_step(cl_search_t *search);

/**
 * Retrieves the value at a given address from the search backup memory.
 * @param dst A pointer to a type matching the search's `type`
 * @param search The search to retrieve the value from
 * @param address The virtual address of the value to retrieve
 */
cl_error cl_search_backup_value(void *dst, const cl_search_t *search,
  cl_addr_t address);

#endif
