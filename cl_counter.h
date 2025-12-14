#ifndef CL_COUNTER_H
#define CL_COUNTER_H

#include "cl_common.h"

bool cl_ctr_store(cl_counter_t *counter, const void *src, cl_value_type type);
bool cl_ctr_store_int(cl_counter_t *counter, int64_t value);
bool cl_ctr_store_float(cl_counter_t *counter, double value);

bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_not_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_lesser(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_lesser_or_equal(const cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_greater_or_equal(const cl_counter_t *left, const cl_counter_t *right);

bool cl_ctr_and(cl_counter_t *counter, const cl_counter_t *value);
bool cl_ctr_or(cl_counter_t *counter, const cl_counter_t *value);
bool cl_ctr_xor(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Returns whether or not a counter is representing a floating point value.
 **/
bool cl_ctr_is_float(const cl_counter_t *counter);

/**
 * Shifts a counter value to the left. Changes counter type to int.
 * @param counter The target counter value.
 * @param value The number of bits to shift left, taken from intval.
 **/
bool cl_ctr_shift_left(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Shifts a counter value to the right. Changes counter type to int.
 * @param counter The target counter value.
 * @param value The number of bits to shift right, taken from intval.
 **/
bool cl_ctr_shift_right(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Flips every bit in a counter. Changes counter type to int.
 **/
bool cl_ctr_complement(cl_counter_t *counter);

/**
 * Adds a value to a counter. Graduates the target counter to float if adding a
 *   float value.
 **/
bool cl_ctr_add(cl_counter_t *left, const cl_counter_t *right);

bool cl_ctr_subtract(cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_multiply(cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_divide(cl_counter_t *left, const cl_counter_t *right);
bool cl_ctr_modulo(cl_counter_t *left, const cl_counter_t *right);

bool cl_ctr_change_type(cl_counter_t *counter, cl_value_type type);

#if CL_TESTS
int cl_ctr_tests(void);
#endif

#endif
