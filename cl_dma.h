#ifndef CL_DMA_H
#define CL_DMA_H

void *cl_dma_alloc(unsigned size, unsigned zero);

void cl_dma_free(void *address);

#endif
