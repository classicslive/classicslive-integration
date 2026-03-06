#include "cl_dma.h"

#include "cl_types.h"

#include <stdlib.h>

void *cl_dma_alloc(unsigned size, unsigned zero)
{
  if (zero)
    return calloc(1, size);
  else
    return malloc(size);
}

void cl_dma_free(void *address)
{
  free(address);
}
