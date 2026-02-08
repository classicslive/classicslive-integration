#ifndef CL_COUNTER_H
#define CL_COUNTER_H

#include "cl_common.h"

/**
 * Stores a value into a counter. The value is taken from the provided source
 * pointer, and the type of the value is specified by the type parameter.
 */
cl_error cl_ctr_store(cl_counter_t *counter, const void *src, cl_value_type type);

/**
 * Stores an integer value into a counter. Changes counter type to int.
 */
cl_error cl_ctr_store_int(cl_counter_t *counter, int64_t value);

/**
 * Stores a floating point value into a counter. Changes counter type to float.
 */
cl_error cl_ctr_store_float(cl_counter_t *counter, double value);

/**
 * Compares two counter values for equality. Returns CL_TRUE if the values are
 * equal.
 */
cl_bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares two counter values for exact equality. Returns CL_TRUE if the raw
 * bit representations of the values are equal.
 */
cl_bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares two counter values for inequality. Returns CL_TRUE if the values are
 * not equal.
 */
cl_bool cl_ctr_not_equal(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares if the left counter value is lesser than the right counter value.
 * Returns CL_TRUE if the left value is lesser.
 */
cl_bool cl_ctr_lesser(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares if the left counter value is greater than the right counter value.
 * Returns CL_TRUE if the left value is greater.
 */
cl_bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares if the left counter value is lesser than or equal to the right
 * counter value. Returns CL_TRUE if the left value is lesser or equal.
 */
cl_bool cl_ctr_lesser_or_equal(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Compares if the left counter value is greater than or equal to the right
 * counter value. Returns CL_TRUE if the left value is greater or equal.
 */
cl_bool cl_ctr_greater_or_equal(const cl_counter_t *left, const cl_counter_t *right);

/**
 * Performs a bitwise AND operation on a counter value with another value. Changes
 * counter type to int.
 * @param counter The target counter value.
 * @param value The value to AND with, taken from intval or floatval depending
 * on the type of the value.
 */
cl_error cl_ctr_and(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Performs a bitwise OR operation on a counter value with another value. Changes
 * counter type to int.
 * @param counter The target counter value.
 * @param value The value to OR with, taken from intval or floatval depending
 * on the type of the value.
 */
cl_error cl_ctr_or(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Performs a bitwise XOR operation on a counter value with another value. Changes
 * counter type to int.
 * @param counter The target counter value.
 * @param value The value to XOR with, taken from intval or floatval depending
 * on the type of the value.
 */
cl_error cl_ctr_xor(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Returns whether or not a counter is representing a floating point value.
 */
cl_bool cl_ctr_is_float(const cl_counter_t *counter);

/**
 * Shifts a counter value to the left. Changes counter type to int.
 * @param counter The target counter value.
 * @param value The number of bits to shift left, taken from intval.
 */
cl_error cl_ctr_shift_left(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Shifts a counter value to the right. Changes counter type to int.
 * @param counter The target counter value.
 * @param value The number of bits to shift right, taken from intval.
 */
cl_error cl_ctr_shift_right(cl_counter_t *counter, const cl_counter_t *value);

/**
 * Flips every bit in a counter. Changes counter type to int.
 */
cl_error cl_ctr_complement(cl_counter_t *counter);

/**
 * Adds a value to a counter. Graduates the target counter to float if adding
 * a float value.
 */
cl_error cl_ctr_add(cl_counter_t *left, const cl_counter_t *right);

/**
 * Subtracts a value from a counter. Graduates the target counter to float if
 * subtracting a float value.
 */
cl_error cl_ctr_subtract(cl_counter_t *left, const cl_counter_t *right);

/**
 * Multiplies a value with a counter. Graduates the target counter to float if
 * multiplying by a float value.
 */
cl_error cl_ctr_multiply(cl_counter_t *left, const cl_counter_t *right);

/**
 * Divides a counter by a value. Graduates the target counter to float if
 * dividing by a float value.
 */
cl_error cl_ctr_divide(cl_counter_t *left, const cl_counter_t *right);

/**
 * Calculates the modulo of a counter by a value. Graduates the target counter
 * to float if modulo by a float value.
 */
cl_error cl_ctr_modulo(cl_counter_t *left, const cl_counter_t *right);

/**
 * Changes the type of a counter.
 */
cl_error cl_ctr_change_type(cl_counter_t *counter, cl_value_type type);

#if CL_TESTS

/**
 * Runs tests for the counter module. Returns 0 if all tests passed, or an
 * error code if any test failed.
 */
int cl_ctr_tests(void);

#endif

#endif
