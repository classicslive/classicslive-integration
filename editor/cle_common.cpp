#include "cle_common.h"

int64_t stringToValue(QString string, bool *ok)
{
  if (string.isEmpty())
    return 0;
  else
  {
    uint8_t base = 0;
    bool negative = false;
    char prefix = string.at(0).toLatin1();

    /* Pop one more if entered value is negative */
    if (prefix == '-')
    {
      negative = true;
      string.remove(0, 1);
      prefix = string.at(0).toLatin1();
    }

    /* Return interpretation of entered number */
    switch (tolower(prefix))
    {
    case 'b':
      /* Binary */
      base = 2;
      break;
    case 'o':
      /* Octal */
      base = 8;
      break;
    case 'd':
      /* Decimal */
      base = 10;
      break;
    case 'h':
    case 'x':
      /* Hexidecimal */
      base = 16;
    }

    if (base)
      string.remove(0, 1);
    else
      base = 10; // TODO: Config option for default base?

    return negative ? 0 - (int64_t)string.toInt(ok, base) :
      string.toUInt(ok, base);
  }
}

cl_error valueToString(char *string, unsigned length, const void *value, cl_value_type type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
    snprintf(string, length, "%02X (%i)", *(int8_t*)value, *(int8_t*)value);
    break;
  case CL_MEMTYPE_UINT8:
    snprintf(string, length, "%02X (%u)", *(uint8_t*)value, *(uint8_t*)value);
    break;
  case CL_MEMTYPE_INT16:
    snprintf(string, length, "%04X (%i)", *(int16_t*)value, *(int16_t*)value);
    break;
  case CL_MEMTYPE_UINT16:
    snprintf(string, length, "%04X (%u)", *(uint16_t*)value, *(uint16_t*)value);
    break;
  case CL_MEMTYPE_INT32:
    snprintf(string, length, "%08X (%i)", *(int32_t*)value, *(int32_t*)value);
    break;
  case CL_MEMTYPE_UINT32:
    snprintf(string, length, "%08X (%u)", *(uint32_t*)value, *(uint32_t*)value);
    break;
  case CL_MEMTYPE_INT64:
    snprintf(string, length, "%08lX (%li)", *(int64_t*)value, *(int64_t*)value);
    break;
  case CL_MEMTYPE_FLOAT:
    snprintf(string, length, "%f", *(float*)value);
    break;
  case CL_MEMTYPE_DOUBLE:
    snprintf(string, length, "%f", *(double*)value);
    break;
  default:
    snprintf(string, length, "???");
    return CL_ERR_PARAMETER_INVALID;
  }

  return CL_OK;
}
