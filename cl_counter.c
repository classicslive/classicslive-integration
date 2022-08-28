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

bool cl_ctr_store(cl_counter_t *counter, void *src, unsigned type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
    return cl_ctr_store_int(counter, *((int8_t*)src));
  case CL_MEMTYPE_UINT8:
    return cl_ctr_store_int(counter, *((uint8_t*)src));
  case CL_MEMTYPE_INT16:
    return cl_ctr_store_int(counter, *((int16_t*)src));
  case CL_MEMTYPE_UINT16:
    return cl_ctr_store_int(counter, *((uint16_t*)src));
  case CL_MEMTYPE_INT32:
    return cl_ctr_store_int(counter, *((int32_t*)src));
  case CL_MEMTYPE_UINT32:
    return cl_ctr_store_int(counter, *((uint32_t*)src));
  case CL_MEMTYPE_INT64:
    return cl_ctr_store_int(counter, *((int64_t*)src));
  case CL_MEMTYPE_FLOAT:
    return cl_ctr_store_float(counter, (double)*((float*)src));
  case CL_MEMTYPE_DOUBLE:
    return cl_ctr_store_float(counter, *((double*)src));
  default:
    return false;
  }

  return true;
}

bool cl_ctr_store_int(cl_counter_t *counter, int64_t value)
{
  counter->intval = value;
  counter->floatval.fp = (double)value;

  return true;
}

bool cl_ctr_store_float(cl_counter_t *counter, double value)
{
  counter->intval = (int64_t)value;
  counter->floatval.fp = value;

  return true;
}

bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) && cl_ctr_is_float(right))
    return fabs(left->floatval.fp - right->floatval.fp) < CL_CTR_EPSILON;
  else
    return left->intval == right->intval;
}

bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right)
{
  return (left->floatval.raw == right->floatval.raw &&
          left->intval == right->intval);
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
    return left->intval < right->intval;
}

bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) && cl_ctr_is_float(right))
    return left->floatval.fp > right->floatval.fp;
  else
    return left->intval > right->intval;
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
  int64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval;

  if (cl_ctr_is_float(counter))
  {
    int64_t temp = counter->floatval.raw;

    temp &= temp_src;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval & temp_src);
}

bool cl_ctr_or(cl_counter_t *counter, const cl_counter_t *value)
{
  int64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval;

  if (cl_ctr_is_float(counter))
  {
    int64_t temp = counter->floatval.raw;

    temp |= temp_src;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval | temp_src);
}

bool cl_ctr_xor(cl_counter_t *counter, const cl_counter_t *value)
{
  int64_t temp_src;

  if (cl_ctr_is_float(value))
    temp_src = value->floatval.raw;
  else
    temp_src = value->intval;

  if (cl_ctr_is_float(counter))
  {
    int64_t temp = counter->floatval.raw;

    temp ^= temp_src;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval ^ temp_src);
}

bool cl_ctr_shift_left(cl_counter_t *counter, const cl_counter_t *value)
{
  return cl_ctr_store_int(counter, (cl_ctr_is_float(counter) ? counter->floatval.raw : counter->intval) << value->intval);
}

bool cl_ctr_shift_right(cl_counter_t *counter, const cl_counter_t *value)
{
  return cl_ctr_store_int(counter, (cl_ctr_is_float(counter) ? counter->floatval.raw : counter->intval) >> value->intval);
}

bool cl_ctr_complement(cl_counter_t *counter)
{
  return cl_ctr_store_int(counter, ~(cl_ctr_is_float(counter) ? counter->floatval.raw : counter->intval));
}

bool cl_ctr_add(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval += right->intval;
  left->floatval.fp += right->floatval.fp;

  return true;
}

bool cl_ctr_subtract(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval -= right->intval;
  left->floatval.fp -= right->floatval.fp;

  return true;
}

bool cl_ctr_multiply(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval *= right->intval;
  left->floatval.fp *= right->floatval.fp;

  return true;
}

bool cl_ctr_divide(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval /= right->intval;
  left->floatval.fp /= right->floatval.fp;

  return true;
}

bool cl_ctr_modulo(cl_counter_t *left, const cl_counter_t *right)
{
  if (cl_ctr_is_float(left) || cl_ctr_is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval %= right->intval;
  left->floatval.fp = fmod(left->floatval.fp, right->floatval.fp);

  return true;
}

/* TODO */
bool cl_ctr_change_type(cl_counter_t *counter, unsigned type)
{
  counter->type = type;

  return true;
}

void cl_ctr_test_add(void)
{
  cl_counter_t a, b;

  cl_ctr_store_int(&a, 2);
  cl_ctr_store_int(&b, 4);
  cl_ctr_add(&a, &b);
  if (a.intval != 6)
    exit(1);
  if (fabs(a.floatval.fp - 6.0) < CL_CTR_EPSILON)
    exit(2);

  cl_ctr_store_float(&a, 10.0);
  cl_ctr_add(&a, &b);
  if (a.intval != 14)
    exit(3);
  if (fabs(a.floatval.fp - 14.0) < CL_CTR_EPSILON)
    exit(4);
}
