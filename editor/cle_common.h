#ifndef CLE_COMMON_H
#define CLE_COMMON_H

extern "C"
{
  #include "../cl_types.h"
}

#include <QString>

int64_t stringToValue(QString string, bool *ok);

/* Output an appropriately formatted C-string representing a memory value. */
void valueToString(char *string, unsigned length, int64_t value,
  cl_value_type memtype);

typedef struct
{
  QString text;
  bool success;
} cle_result_t;

#endif
