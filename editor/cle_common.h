#ifndef CLE_COMMON_H
#define CLE_COMMON_H

extern "C"
{
  #include "../cl_types.h"
}

#include <QString>

QString formatBytes(double bytes);

/**
 * Parses a number the user typed. Accepts an optional sign followed by an
 * optional base prefix: "0x", "x" or "h" for hexadecimal, "0b" or "b" for
 * binary, "0o" or "o" for octal, and "d" for decimal, in either case.
 * The value is parsed at full width and returned as the bit pattern it
 * occupies, so a negative or oversized entry truncates into whatever width
 * the caller stores it in.
 * A prefix letter that is a valid digit in `base` is read as a digit instead,
 * so "0b" stays hexadecimal 11 in a field that already reads as hex.
 * @param string The text the user entered
 * @param ok Set to whether the entire string parsed, or NULL to ignore
 * @param base The base to read the digits in when no prefix is given
 */
int64_t stringToValueBase(QString string, bool *ok, int base);

/** As stringToValueBase, reading unprefixed entries as decimal. */
int64_t stringToValue(QString string, bool *ok);

/**
 * Parses a floating-point number the user typed, reporting how many digits
 * were given after the decimal point so that a search can match only as
 * precisely as it was asked to. Base prefixes are not accepted.
 * @param string The text the user entered
 * @param decimals Set to the count of digits after the point, or NULL
 * @param ok Set to whether the entire string parsed, or NULL to ignore
 */
double stringToFloat(QString string, unsigned *decimals, bool *ok);

/* Output an appropriately formatted C-string representing a memory value. */
cl_error valueToString(char *string, unsigned length, const void *value, cl_value_type type);

typedef struct
{
  QString text;
  bool success;
} cle_result_t;

#endif
