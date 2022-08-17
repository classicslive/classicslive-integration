#ifndef CL_COUNTER_H
#define CL_COUNTER_H

#include "cl_types.h"

typedef struct cl_counter_t
{
   int64_t intval;
   double floatval;
   unsigned type;
} cl_counter_t;

bool cl_ctr_store(cl_counter_t *counter, void *src, unsigned type);
bool cl_ctr_store_int(cl_counter_t *counter, int64_t value);
bool cl_ctr_store_float(cl_counter_t *counter, double value);

bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_not_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_lesser(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_lesser_or_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_greater_or_equal(const cl_counter_t *left, const cl_counter_t *right);

bool cl_ctr_and(cl_counter_t *counter, int64_t *value);
bool cl_ctr_or(cl_counter_t *counter, int64_t *value);
bool cl_ctr_xor(cl_counter_t *counter, int64_t *value);
bool cl_ctr_shift_left(cl_counter_t *counter, int64_t *value);
bool cl_ctr_shift_right(cl_counter_t *counter, int64_t *value);
bool cl_ctr_complement(cl_counter_t *counter);

bool cl_ctr_add(cl_counter_t *left, cl_counter_t *right);
bool cl_ctr_subtract(cl_counter_t *left, cl_counter_t *right);
bool cl_ctr_multiply(cl_counter_t *left, cl_counter_t *right);
bool cl_ctr_divide(cl_counter_t *left, cl_counter_t *right);
bool cl_ctr_modulo(cl_counter_t *left, cl_counter_t *right);

#endif
