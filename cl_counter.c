#include "cl_common.h"
#include "cl_counter.h"
#include "cl_memory.h"

#include <math.h>
#include <string.h>

#define CL_CTR_EPSILON 0.005

bool cl_ctr_is_float(const cl_counter_t *counter)
{
  if (counter->type == CL_MEMTYPE_DOUBLE || counter->type == CL_MEMTYPE_FLOAT)
    return true;
  else
    return false;
}

bool cl_ctr_store(cl_counter_t *counter, const void *src, cl_value_type type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
    return cl_ctr_store_int(counter, *((const int8_t*)src));
  case CL_MEMTYPE_UINT8:
    return cl_ctr_store_int(counter, *((const uint8_t*)src));
  case CL_MEMTYPE_INT16:
    return cl_ctr_store_int(counter, *((const int16_t*)src));
  case CL_MEMTYPE_UINT16:
    return cl_ctr_store_int(counter, *((const uint16_t*)src));
  case CL_MEMTYPE_INT32:
    return cl_ctr_store_int(counter, *((const int32_t*)src));
  case CL_MEMTYPE_UINT32:
    return cl_ctr_store_int(counter, *((const uint32_t*)src));
  case CL_MEMTYPE_INT64:
    return cl_ctr_store_int(counter, *((const int64_t*)src));
  case CL_MEMTYPE_FLOAT:
    return cl_ctr_store_float(counter, (double)*((const float*)src));
  case CL_MEMTYPE_DOUBLE:
    return cl_ctr_store_float(counter, *((const double*)src));
  default:
    return false;
  }
}

/** @todo Make sure the commented lines still work */
bool cl_ctr_store_int(cl_counter_t *counter, int64_t value)
{
  counter->intval.i64 = value;
  counter->floatval.fp = (double)value;
  counter->type = CL_MEMTYPE_INT64;

  return true;
}

bool cl_ctr_store_float(cl_counter_t *counter, double value)
{
  counter->intval.i64 = (int64_t)value;
  counter->floatval.fp = value;
  counter->type = CL_MEMTYPE_DOUBLE;

  return true;
}

bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) && cl_ctr_is_float(right))
    return fabs(left->floatval.fp - right->floatval.fp) < CL_CTR_EPSILON;
  else
    return left->intval.i64 == right->intval.i64;
}

bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right)
{
  return (left->floatval.raw == right->floatval.raw &&
          left->intval.i64 == right->intval.i64);
}

bool cl_ctr_not_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  return !cl_ctr_equal(left, right);
}

bool cl_ctr_lesser(const cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) && cl_ctr_is_float(right))
    return left->floatval.fp < right->floatval.fp;
  else
    return left->intval.i64 < right->intval.i64;
}

bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) && cl_ctr_is_float(right))
    return left->floatval.fp > right->floatval.fp;
  else
    return left->intval.i64 > right->intval.i64;
}

bool cl_ctr_lesser_or_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  return cl_ctr_lesser(left, right) || cl_ctr_equal(left, right);
}

bool cl_ctr_greater_or_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  return cl_ctr_greater(left, right) || cl_ctr_equal(left, right);
}

bool cl_ctr_and(cl_counter_t *counter, const cl_counter_t *value)
{
  uint64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval.raw;

  if (cl_ctr_is_float(counter))
  {
    uint64_t temp = counter->floatval.raw & temp_src;
    double temp_fp;

    memcpy(&temp_fp, &temp, sizeof(temp_fp));

    return cl_ctr_store_float(counter, temp_fp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval.raw & temp_src);
}

bool cl_ctr_or(cl_counter_t *counter, const cl_counter_t *value)
{
  uint64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval.raw;

  if (cl_ctr_is_float(counter))
  {
    uint64_t temp = counter->floatval.raw | temp_src;
    double temp_fp;

    memcpy(&temp_fp, &temp, sizeof(temp_fp));

    return cl_ctr_store_float(counter, temp_fp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval.raw | temp_src);
}

bool cl_ctr_xor(cl_counter_t *counter, const cl_counter_t *value)
{
  uint64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval.raw;

  if (cl_ctr_is_float(counter))
  {
    uint64_t temp = counter->floatval.raw ^ temp_src;
    double temp_fp;

    memcpy(&temp_fp, &temp, sizeof(temp_fp));

    return cl_ctr_store_float(counter, temp_fp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval.raw ^ temp_src);
}

bool cl_ctr_shift_left(cl_counter_t *counter, const cl_counter_t *value)
{
  if (cl_ctr_is_float(counter))
  {
    counter->floatval.raw <<= value->intval.raw;
    return cl_ctr_store_int(counter, counter->floatval.raw);
  }
  else
    return cl_ctr_store_int(counter, counter->intval.raw << value->intval.raw);
}

bool cl_ctr_shift_right(cl_counter_t *counter, const cl_counter_t *value)
{
  if (cl_ctr_is_float(counter))
  {
    counter->floatval.raw >>= value->intval.raw;
    return cl_ctr_store_int(counter, counter->floatval.raw);
  }
  else
    return cl_ctr_store_int(counter, counter->intval.raw >> value->intval.raw);
}

bool cl_ctr_complement(cl_counter_t *counter)
{
  return cl_ctr_store_int(counter,
    ~(cl_ctr_is_float(counter) ? counter->floatval.raw : counter->intval.raw));
}

bool cl_ctr_add(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval.i64 += right->intval.i64;
  left->floatval.fp += right->floatval.fp;

  return true;
}

bool cl_ctr_subtract(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval.i64 -= right->intval.i64;
  left->floatval.fp -= right->floatval.fp;

  return true;
}

bool cl_ctr_multiply(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval.i64 *= right->intval.i64;
  left->floatval.fp *= right->floatval.fp;

  return true;
}

bool cl_ctr_divide(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval.i64 /= right->intval.i64;
  left->floatval.fp /= right->floatval.fp;

  return true;
}

bool cl_ctr_modulo(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval.i64 %= right->intval.i64;
  left->floatval.fp = fmod(left->floatval.fp, right->floatval.fp);

  return true;
}

/* TODO */
bool cl_ctr_change_type(cl_counter_t *counter, cl_value_type type)
{
  counter->type = type;

  return true;
}

#if CL_TESTS

#include <stdio.h>

static void cl_ctr_test_add(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 2);
  cl_ctr_store_int(&b, 4);
  cl_ctr_add(&a, &b);
  if (a.intval.i64 != 6)
    CL_TEST_FAIL(1);
  if (fabs(a.floatval.fp - 6.0) < CL_CTR_EPSILON)
    CL_TEST_FAIL(2);

  cl_ctr_store_float(&a, 10.0);
  cl_ctr_add(&a, &b);
  if (a.intval.i64 != 14)
    exit(3);
  if (fabs(a.floatval.fp - 14.0) < CL_CTR_EPSILON)
    exit(4);
}

static void cl_ctr_test_subtract(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 10);
  cl_ctr_store_int(&b, 4);
  cl_ctr_subtract(&a, &b);
  if (a.intval.i64 != 6)
    exit(1);
  if (fabs(a.floatval.fp - 6.0) > CL_CTR_EPSILON)
    exit(2);

  cl_ctr_store_float(&a, 15.0);
  cl_ctr_subtract(&a, &b);
  if (a.intval.i64 != 11)
    exit(3);
  if (fabs(a.floatval.fp - 11.0) > CL_CTR_EPSILON)
    exit(4);
}

static void cl_ctr_test_multiply(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 3);
  cl_ctr_store_int(&b, 5);
  cl_ctr_multiply(&a, &b);
  if (a.intval.i64 != 15)
    exit(1);
  if (fabs(a.floatval.fp - 15.0) > CL_CTR_EPSILON)
    exit(2);

  cl_ctr_store_float(&a, 2.0);
  cl_ctr_multiply(&a, &b);
  if (a.intval.i64 != 10)
    exit(3);
  if (fabs(a.floatval.fp - 10.0) > CL_CTR_EPSILON)
    exit(4);
}

static void cl_ctr_test_divide(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 20);
  cl_ctr_store_int(&b, 4);
  cl_ctr_divide(&a, &b);
  if (a.intval.i64 != 5)
    exit(1);
  if (fabs(a.floatval.fp - 5.0) > CL_CTR_EPSILON)
    exit(2);

  cl_ctr_store_float(&a, 30.0);
  cl_ctr_divide(&a, &b);
  if (a.intval.i64 != 7)
    exit(3);
  if (fabs(a.floatval.fp - 7.5) > CL_CTR_EPSILON)
    exit(4);
}

static void cl_ctr_test_modulo(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 10);
  cl_ctr_store_int(&b, 3);
  cl_ctr_modulo(&a, &b);
  if (a.intval.i64 != 1)
    exit(1);
  if (fabs(a.floatval.fp - 1.0) > CL_CTR_EPSILON)
    exit(2);

  cl_ctr_store_float(&a, 15.5);
  cl_ctr_store_float(&b, 4.0);
  cl_ctr_modulo(&a, &b);
  if (a.intval.i64 != 3)
    exit(3);
  if (fabs(a.floatval.fp - 3.5) > CL_CTR_EPSILON)
    exit(4);
}

static void cl_ctr_test_shift_left(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 10);
  cl_ctr_store_int(&b, 2);
  cl_ctr_shift_left(&a, &b);
  if (a.intval.i64 != 40)
    exit(1);

  cl_ctr_store_float(&a, 15.0);
  cl_ctr_shift_left(&a, &b);
  if (a.intval.i64 != 60)
    exit(2);
}

static void cl_ctr_test_shift_right(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 40);
  cl_ctr_store_int(&b, 2);
  cl_ctr_shift_right(&a, &b);
  if (a.intval.i64 != 10)
    exit(1);

  cl_ctr_store_float(&a, 64.0);
  cl_ctr_shift_right(&a, &b);
  if (a.intval.i64 != 16)
    exit(2);
}

static void cl_ctr_test_complement(void)
{
  cl_counter_t a;

  cl_ctr_store_int(&a, 10);
  cl_ctr_complement(&a);
  if (a.intval.i64 != ~10)
    exit(1);
}

int cl_ctr_tests(void)
{
  cl_ctr_test_add();
  cl_ctr_test_subtract();
  cl_ctr_test_multiply();
  cl_ctr_test_divide();
  cl_ctr_test_modulo();
  cl_ctr_test_shift_left();
  cl_ctr_test_shift_right();
  cl_ctr_test_complement();

  return 1;
}

#endif
