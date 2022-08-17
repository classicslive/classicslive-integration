#include "cl_common.h"
#include "cl_counter.h"
#include "cl_memory.h"

#include <math.h>
#include <string.h>

#define CL_CTR_EPSILON 0.005

static bool is_float(const cl_counter_t *counter)
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
  {
    int8_t value = *((int8_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_UINT8:
  {
    uint8_t value = *((uint8_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_INT16:
  {
    int16_t value = *((int16_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_UINT16:
  {
    uint16_t value = *((uint16_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_INT32:
  {
    int32_t value = *((int32_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_UINT32:
  {
    uint32_t value = *((uint32_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_INT64:
  {
    int64_t value = *((int64_t*)src);
    return cl_ctr_store_int(counter, value);
  }
  case CL_MEMTYPE_FLOAT:
  {
    float value = *((float*)src);
    return cl_ctr_store_float(counter, (double)value);
  }
  case CL_MEMTYPE_DOUBLE:
  {
    double value = *((double*)src);
    return cl_ctr_store_float(counter, value);
  }
  default:
    return false;
  }

  return true;
}

bool cl_ctr_store_int(cl_counter_t *counter, int64_t value)
{
  counter->intval = value;
  counter->floatval = (double)value;

  return true;
}

bool cl_ctr_store_float(cl_counter_t *counter, double value)
{
  counter->intval = (int64_t)value;
  counter->floatval = value;

  return true;
}

bool cl_ctr_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  if (is_float(left) && is_float(right))
    return fabs(left->floatval - right->floatval) < CL_CTR_EPSILON;
  else
    return left->intval == right->intval;
}

bool cl_ctr_equal_exact(const cl_counter_t *left, const cl_counter_t *right)
{
  return left->floatval == right->floatval && left->intval == right->intval;
}

bool cl_ctr_not_equal(const cl_counter_t *left, const cl_counter_t *right)
{
  return !cl_ctr_equal(left, right);
}

bool cl_ctr_lesser(const cl_counter_t *left, const cl_counter_t *right)
{
  if (is_float(left) && is_float(right))
    return left->floatval < right->floatval;
  else
    return left->intval < right->intval;
}

bool cl_ctr_greater(const cl_counter_t *left, const cl_counter_t *right)
{
  if (is_float(left) && is_float(right))
    return left->floatval > right->floatval;
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

bool cl_ctr_and(cl_counter_t *counter, int64_t *value)
{
  if (is_float(counter))
  {
    int64_t temp;

    memcpy(&temp, &counter->floatval, sizeof(temp));
    temp &= *value;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval & *value);
}

bool cl_ctr_or(cl_counter_t *counter, int64_t *value)
{
  if (is_float(counter))
  {
    int64_t temp;

    memcpy(&temp, &counter->floatval, sizeof(temp));
    temp |= *value;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval | *value);
}

bool cl_ctr_xor(cl_counter_t *counter, int64_t *value)
{
  if (is_float(counter))
  {
    int64_t temp;

    memcpy(&temp, &counter->floatval, sizeof(temp));
    temp ^= *value;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, counter->intval ^ *value);
}

bool cl_ctr_shift_left(cl_counter_t *counter, int64_t *value)
{
  if (is_float(counter))
  {
    counter->type = CL_MEMTYPE_INT64;
    memcpy(&counter->intval, &counter->floatval, sizeof(counter->intval));
  }
  return cl_ctr_store_int(counter, counter->intval << *value);
}

bool cl_ctr_shift_right(cl_counter_t *counter, int64_t *value)
{
  if (is_float(counter))
  {
    counter->type = CL_MEMTYPE_INT64;
    memcpy(&counter->intval, &counter->floatval, sizeof(counter->intval));
  }
  return cl_ctr_store_int(counter, counter->intval >> *value);
}

bool cl_ctr_complement(cl_counter_t *counter)
{
  if (is_float(counter))
  {
    int64_t temp;

    memcpy(&temp, &counter->floatval, sizeof(temp));
    temp = ~temp;
    return cl_ctr_store_float(counter, *(double*)&temp);
  }
  else
    return cl_ctr_store_int(counter, ~counter->intval);
}

bool cl_ctr_add(cl_counter_t *left, cl_counter_t *right)
{
  if (is_float(left) || is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval = left->intval + right->intval;
  left->floatval = left->floatval + right->floatval;

  return true;
}

bool cl_ctr_subtract(cl_counter_t *left, cl_counter_t *right)
{
  if (is_float(left) || is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval = left->intval - right->intval;
  left->floatval = left->floatval - right->floatval;

  return true;
}

bool cl_ctr_multiply(cl_counter_t *left, cl_counter_t *right)
{
  if (is_float(left) || is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval = left->intval * right->intval;
  left->floatval = left->floatval * right->floatval;

  return true;
}

bool cl_ctr_divide(cl_counter_t *left, cl_counter_t *right)
{
  if (is_float(left) || is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval = left->intval / right->intval;
  left->floatval = left->floatval / right->floatval;

  return true;
}

bool cl_ctr_modulo(cl_counter_t *left, cl_counter_t *right)
{
  if (is_float(left) || is_float(right))
    left->type = CL_MEMTYPE_DOUBLE;
  left->intval = left->intval % right->intval;
  left->floatval = fmod(left->floatval, right->floatval);

  return true;
}
