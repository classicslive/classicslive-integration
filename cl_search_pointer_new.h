#ifndef CL_SEARCH_POINTER_NEW_H
#define CL_SEARCH_POINTER_NEW_H

#include "cl_types.h"

/**
 * An opaque handle representing a target value for searches.
 */
typedef union
{
  unsigned char raw[8];
  double _align;
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

  /** Whether to use the target value as a value in comparisons */
  unsigned target_none;
} cl_search_parameters_t;

/**
 * A single pointer search result containing the pointer chain and values.
 */
typedef struct
{
  /* The initial address where the pointer chain starts */
  cl_addr_t address_initial;

  /* The final address after following all pointers */
  cl_addr_t address_final;

  /* The current value at the final address */
  cl_search_target_t value_current;

  /* The previous value at the final address */
  cl_search_target_t value_previous;

  /* The offsets applied at each pointer level */
  cl_addr_t offsets[CL_POINTER_MAX_PASSES];
} cl_pointersearch_result_t;

typedef struct
{
  void *chunk;

  cl_addr_t start;

  cl_addr_t size;
} cl_pointersearch_deepcopy_chunk_t;

/**
 * A deep copy of guest process memory
 */
typedef struct
{
  cl_pointersearch_deepcopy_chunk_t *chunks;
  unsigned chunk_count;
} cl_pointersearch_deepcopy_t;

/**
 * The main pointer search structure.
 * Searches for pointer chains that lead to a target address.
 */
typedef struct
{
  /* Search parameters */
  cl_search_parameters_t params;

  /* Number of pointer dereferences in the chain */
  unsigned passes;

  /* Maximum offset range to search for at each level */
  cl_addr_t range;

  /* Array of found pointer chains */
  cl_pointersearch_result_t *results;

  /* Number of results found */
  cl_addr_t result_count;

  /* Maximum number of results to store */
  cl_addr_t max_results;
} cl_pointersearch_t;

/**
 * Frees memory allocated for a pointer search.
 * @param search A pointer to the pointer search to free
 */
cl_error cl_pointersearch_free(cl_pointersearch_t *search);

/**
 * Changes the comparison type used in the search.
 * @param search A pointer to the search to modify
 * @param compare_type The new comparison type to use
 */
cl_error cl_pointersearch_change_compare_type(cl_pointersearch_t *search,
  cl_compare_type compare_type);

/**
 * Changes the value type used in the search. This cannot be used once the
 * search has begun (i.e. after calling `cl_pointersearch_step`).
 * @param search A pointer to the search to modify
 * @param type The new value type to use
 */
cl_error cl_pointersearch_change_value_type(cl_pointersearch_t *search, cl_value_type type);

/**
 * Changes the target value used in the search.
 * @param search A pointer to the search to modify
 * @param value A pointer to the new target value to use, or NULL to compare
 *   against the previous value instead
 */
cl_error cl_pointersearch_change_target(cl_pointersearch_t *search, const void *value);

/**
 * Initializes a pointer search to find chains leading to a target address.
 * @param search A pointer to the search structure to initialize
 * @param address The target address to find pointers to
 * @param value_type The type of value at the target address
 * @param passes The number of pointer dereferences to search through
 * @param range The maximum offset range for each pointer level
 * @param max_results The maximum number of results to store
 * @return CL_OK on success, or an error code on failure
 */
cl_error cl_pointersearch_init(cl_pointersearch_t *search, cl_addr_t address,
  cl_value_type value_type, unsigned passes, cl_addr_t range,
  cl_addr_t max_results);

/**
 * Filters pointer search results based on value comparisons.
 * @param search A pointer to the pointer search
 * @return The number of matching results
 */
cl_addr_t cl_pointersearch_step(cl_pointersearch_t *search);

/**
 * Updates the current values for all pointer search results.
 * Resolves each pointer chain and reads the current value.
 * @param search A pointer to the pointer search to update
 */
cl_error cl_pointersearch_update(cl_pointersearch_t *search);

#endif
