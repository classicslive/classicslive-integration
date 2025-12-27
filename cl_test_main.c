#include "cl_test.h"

int main(void)
{
  cl_error error;
  unsigned i;

  for (i = 0; i < 5; i++)
  {
    error = cl_test();
    if (error != CL_OK)
      return error;
  }

  return CL_OK;
}
