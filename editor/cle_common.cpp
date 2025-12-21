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

void valueToString(char *string, unsigned length, int64_t value,
  cl_value_type memtype)
{
  switch (memtype)
  {
  case CL_MEMTYPE_INT8:
    snprintf(string, length, "%02lX (%li)", value, value);
    break;
  case CL_MEMTYPE_UINT8:
    snprintf(string, length, "%02lX (%lu)", value, value);
    break;
  case CL_MEMTYPE_INT16:
    snprintf(string, length, "%04lX (%li)", value, value);
    break;
  case CL_MEMTYPE_UINT16:
    snprintf(string, length, "%04lX (%lu)", value, value);
      break;
  case CL_MEMTYPE_INT32:
    snprintf(string, length, "%08lX (%li)", value, value);
    break;
  case CL_MEMTYPE_UINT32:
    snprintf(string, length, "%08lX (%lu)", value, value);
    break;
  case CL_MEMTYPE_FLOAT:
    snprintf(string, length, "%08lX (%f)", value, *((float*)(&value)));
    break;
  default:
    snprintf(string, length, "%08lX", value);
    break;
  }
}
