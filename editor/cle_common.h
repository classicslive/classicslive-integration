#ifndef CLE_COMMON_H
#define CLE_COMMON_H

extern "C"
{
  #include "../cl_memory.h"
}

#include <QString>

uint32_t stringToValue(QString string, bool *ok);

/* Output an appropriately formatted C-string representing a memory value. */
void valueToString(char *string, uint8_t length, uint32_t value, 
  uint8_t memtype);

typedef struct
{
  QString text;
  bool success;
} cle_result_t;

#endif
