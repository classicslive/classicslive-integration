#ifndef CLE_COMMON_H
#define CLE_COMMON_H

extern "C"
{
  #include "../cl_types.h"
}

#include <QString>

int64_t stringToValue(QString string, bool *ok);

/* Output an appropriately formatted C-string representing a memory value. */
cl_error valueToString(char *string, unsigned length, const void *value, cl_value_type type);

typedef struct
{
  QString text;
  bool success;
} cle_result_t;

#endif
