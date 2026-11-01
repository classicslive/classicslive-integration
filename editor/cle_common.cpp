#include "cle_common.h"

QString formatBytes(double bytes)
{
  const double kb = 1024.0;
  const double mb = kb * 1024.0;
  const double gb = mb * 1024.0;

  if (bytes >= gb)
    return QString("%1 GB").arg(bytes / gb, 0, 'f', 2);
  else if (bytes >= mb)
    return QString("%1 MB").arg(bytes / mb, 0, 'f', 2);
  else
    return QString("%1 KB").arg(bytes / kb, 0, 'f', 2);
}

/**
 * Removes a base prefix from the front of an entry and returns the base it
 * named, or `base` if there was none. Letters that are valid digits in the
 * base already in effect are left alone to be read as digits.
 */
static int cleTakeBasePrefix(QString &body, int base)
{
  const bool hex = (base == 16);

  if (body.startsWith(QLatin1String("0x"), Qt::CaseInsensitive))
  {
    body.remove(0, 2);
    return 16;
  }
  else if (!hex && body.startsWith(QLatin1String("0b"), Qt::CaseInsensitive))
  {
    body.remove(0, 2);
    return 2;
  }
  else if (!hex && body.startsWith(QLatin1String("0o"), Qt::CaseInsensitive))
  {
    body.remove(0, 2);
    return 8;
  }
  else if (body.length() > 1)
  {
    switch (body.at(0).toLower().toLatin1())
    {
    case 'x':
    case 'h':
      body.remove(0, 1);
      return 16;
    case 'b':
      if (hex)
        break;
      body.remove(0, 1);
      return 2;
    case 'o':
      if (hex)
        break;
      body.remove(0, 1);
      return 8;
    case 'd':
      if (hex)
        break;
      body.remove(0, 1);
      return 10;
    default:
      break;
    }
  }

  return base;
}

int64_t stringToValueBase(QString string, bool *ok, int base)
{
  QString body = string.trimmed();
  bool negative = false;
  bool converted = false;
  qulonglong magnitude;
  int i;

  if (ok)
    *ok = false;

  if (body.startsWith(QLatin1Char('-')))
  {
    negative = true;
    body.remove(0, 1);
  }
  else if (body.startsWith(QLatin1Char('+')))
    body.remove(0, 1);

  base = cleTakeBasePrefix(body, base);
  if (body.isEmpty())
    return 0;

  /* Qt's conversion skips leading whitespace and accepts a sign among the
     digits, so what is left after the prefix is vetted here instead */
  for (i = 0; i < body.length(); i++)
  {
    const int character = body.at(i).unicode();
    int digit;

    if (character >= '0' && character <= '9')
      digit = character - '0';
    else if (character >= 'a' && character <= 'f')
      digit = character - 'a' + 10;
    else if (character >= 'A' && character <= 'F')
      digit = character - 'A' + 10;
    else
      return 0;

    if (digit >= base)
      return 0;
  }

  magnitude = body.toULongLong(&converted, base);
  if (!converted)
    return 0;

  if (ok)
    *ok = true;

  /* Negation is done unsigned so the most negative value stays in range */
  return negative ? (int64_t)(0ULL - magnitude) : (int64_t)magnitude;
}

int64_t stringToValue(QString string, bool *ok)
{
  return stringToValueBase(string, ok, 10);
}

double stringToFloat(QString string, unsigned *decimals, bool *ok)
{
  const QString body = string.trimmed();
  double value;
  bool converted = false;

  if (decimals)
  {
    int point = body.indexOf(QLatin1Char('.'));

    *decimals = 0;
    while (point >= 0 && ++point < body.length() &&
           body.at(point).isDigit())
      (*decimals)++;
  }

  value = body.toDouble(&converted);
  if (ok)
    *ok = converted;

  return converted ? value : 0.0;
}

/**
 * Writes a memory value in the same notation stringToValue reads back, so a
 * value shown in the interface can be pasted straight into a search. Signed
 * types are reinterpreted at their own width for the hexadecimal half, which
 * would otherwise be sign-extended out to a full int by the promotion.
 */
cl_error valueToString(char *string, unsigned length, const void *value,
  cl_value_type type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:
    snprintf(string, length, "0x%02X (%d)",
      *(const uint8_t*)value, *(const int8_t*)value);
    break;
  case CL_MEMTYPE_UINT8:
    snprintf(string, length, "0x%02X (%u)",
      *(const uint8_t*)value, *(const uint8_t*)value);
    break;
  case CL_MEMTYPE_INT16:
    snprintf(string, length, "0x%04X (%d)",
      *(const uint16_t*)value, *(const int16_t*)value);
    break;
  case CL_MEMTYPE_UINT16:
    snprintf(string, length, "0x%04X (%u)",
      *(const uint16_t*)value, *(const uint16_t*)value);
    break;
  case CL_MEMTYPE_INT32:
    snprintf(string, length, "0x%08X (%d)",
      *(const uint32_t*)value, *(const int32_t*)value);
    break;
  case CL_MEMTYPE_UINT32:
    snprintf(string, length, "0x%08X (%u)",
      *(const uint32_t*)value, *(const uint32_t*)value);
    break;
  case CL_MEMTYPE_INT64:
    snprintf(string, length, "0x" CL_FX64 " (" CL_FS64 ")",
      *(const uint64_t*)value, *(const int64_t*)value);
    break;
  case CL_MEMTYPE_FLOAT:
    snprintf(string, length, "%g", *(const float*)value);
    break;
  case CL_MEMTYPE_DOUBLE:
    snprintf(string, length, "%g", *(const double*)value);
    break;
  default:
    snprintf(string, length, "???");
    return CL_ERR_PARAMETER_INVALID;
  }

  return CL_OK;
}
