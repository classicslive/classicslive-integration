#ifndef CL_SEARCH_H
#define CL_SEARCH_H

#include "cl_types.h"

#define CL_POINTER_MAX_PASSES 4

typedef struct cl_searchbank_t
{
   cl_membank_t *bank;
   uint8_t *backup;
   uint8_t *valid;
   bool any_valid;
} cl_searchbank_t;

typedef struct cl_search_t
{
   cl_searchbank_t *searchbanks;
   uint8_t          searchbank_count;
   uint8_t          size;
   uint32_t         matches;
} cl_search_t;

typedef struct cl_pointerresult_t
{
   uint32_t  address_initial;
   uint32_t  address_final;
   uint32_t  value_current;
   uint32_t  value_previous;
   int32_t   offsets[CL_POINTER_MAX_PASSES];
} cl_pointerresult_t;

typedef struct cl_pointersearch_t
{
   uint8_t             passes;
   uint32_t            range; 
   cl_pointerresult_t *results;
   uint32_t            result_count;
   uint8_t             size;
} cl_pointersearch_t;

bool cl_read_search (uint32_t *value, cl_search_t *search, 
   cl_searchbank_t *bank, uint32_t address, uint8_t size);

uint32_t cl_search_ascii (cl_search_t *search, const char *needle, uint8_t length);
bool     cl_search_free  (cl_search_t *search);
bool     cl_search_init  (cl_search_t *search);
void     cl_search_remove(cl_search_t *search, uint32_t address);
bool     cl_search_reset (cl_search_t *search);
uint32_t cl_search_step  (cl_search_t *search, void *value, uint8_t size, 
   uint8_t type, bool is_float);

bool cl_pointersearch_free    (cl_pointersearch_t *search);
bool cl_pointersearch_init    (cl_pointersearch_t *search, 
   uint32_t address, uint8_t size, uint8_t passes, uint32_t range, uint32_t max_results);
uint32_t cl_pointersearch_step(cl_pointersearch_t *search, uint32_t *value,
   uint8_t size, uint8_t type);

/*
   Updates the "value_current" variable in every result in a pointersearch.
*/
void cl_pointersearch_update(cl_pointersearch_t *search);

#endif
