#include "cl_config.h"

static const cl_config_t default_config =
{
  "https://doggylongface.com/classicslive/"
};

cl_config_t cl_default_config(void)
{
  return default_config;
}
