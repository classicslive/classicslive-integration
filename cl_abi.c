#include "cl_abi.h"

static const cl_abi_t *cl_g_abi = NULL;

cl_error cl_abi_register(const cl_abi_t *abi)
{
  if (!abi)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    if (abi->version < CL_ABI_VERSION)
      return CL_ERR_PARAMETER_INVALID;
    else if (/* !abi->functions.core.display_message || */
        !abi->functions.core.install_memory_regions ||
        !abi->functions.core.library_name ||
        !abi->functions.core.network_post ||
        /* !abi->functions.core.set_pause || */
        !abi->functions.core.thread ||
        !abi->functions.core.user_data)
      return CL_ERR_PARAMETER_INVALID;
#if CL_EXTERNAL_MEMORY
    else if (!abi->functions.external.read_buffer ||
             !abi->functions.external.read_value ||
             !abi->functions.external.write_buffer ||
             !abi->functions.external.write_value)
      return CL_ERR_PARAMETER_INVALID;
#endif
    else
      cl_g_abi = abi;

    return CL_OK;
  }
}

cl_error cl_abi_display_message(unsigned level, const char *msg)
{
  if (cl_g_abi && cl_g_abi->functions.core.display_message && msg)
    return cl_g_abi->functions.core.display_message(level, msg);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_install_memory_regions(cl_memory_region_t **regions,
                                       unsigned *region_count)
{
  if (cl_g_abi &&
      cl_g_abi->functions.core.install_memory_regions &&
      regions && region_count)
    return cl_g_abi->functions.core.install_memory_regions(regions, region_count);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_library_name(const char **name)
{
  if (cl_g_abi && cl_g_abi->functions.core.library_name && name)
    return cl_g_abi->functions.core.library_name(name);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_network_post(const char *url, char *data,
                             cl_network_cb_t callback, void *userdata)
{
  if (cl_g_abi && cl_g_abi->functions.core.network_post && url && data)
    return cl_g_abi->functions.core.network_post(url, data, callback, userdata);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_set_pause(unsigned mode)
{
  if (cl_g_abi && cl_g_abi->functions.core.set_pause)
    return cl_g_abi->functions.core.set_pause(mode);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_thread(cl_task_t *task)
{
  if (cl_g_abi && cl_g_abi->functions.core.thread && task)
    return cl_g_abi->functions.core.thread(task);
  else
    return CL_ERR_PARAMETER_NULL;
}

cl_error cl_abi_user_data(cl_user_t *user, unsigned index)
{
  if (cl_g_abi && cl_g_abi->functions.core.user_data && user)
    return cl_g_abi->functions.core.user_data(user, index);
  else
    return CL_ERR_PARAMETER_NULL;
}

#if CL_EXTERNAL_MEMORY

cl_error cl_abi_external_read_buffer(void *dest, cl_addr_t address,
                              unsigned size, unsigned *read)
{
#if CL_SAFETY
  if (!dest || !read || !cl_g_abi || !cl_g_abi->functions.external.read_buffer)
    return CL_ERR_PARAMETER_NULL;
#endif
  return cl_g_abi->functions.external.read_buffer(dest, address, size, read);
}

cl_error cl_abi_external_read_value(void *dest, cl_addr_t address,
                              cl_value_type type)
{
#if CL_SAFETY
  if (!dest || !cl_g_abi || !cl_g_abi->functions.external.read_value)
    return CL_ERR_PARAMETER_NULL;
#endif
  return cl_g_abi->functions.external.read_value(dest, address, type);
}

cl_error cl_abi_external_write_buffer(const void *src, cl_addr_t address,
                               unsigned size, unsigned *written)
{
#if CL_SAFETY
  if (!src || !written || !cl_g_abi || !cl_g_abi->functions.external.write_buffer)
    return CL_ERR_PARAMETER_NULL;
#endif
  return cl_g_abi->functions.external.write_buffer(src, address, size, written);
}

cl_error cl_abi_external_write_value(const void *src, cl_addr_t address,
                               cl_value_type type)
{
#if CL_SAFETY
  if (!src || !cl_g_abi || !cl_g_abi->functions.external.write_value)
    return CL_ERR_PARAMETER_NULL;
#endif
  return cl_g_abi->functions.external.write_value(src, address, type);
}

#endif
