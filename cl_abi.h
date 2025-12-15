#ifndef CL_ABI_H
#define CL_ABI_H

/**
 * @file cl_abi.h
 * The functions prototyped below are implementation-specific callbacks that
 * must be written for any program intended to be used with Classics Live. 
 */

#include "cl_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CL_ABI_VERSION 1

/**
 * Signals to the frontend to display a message.
 * Can be stubbed, but not recommended.
 * @param level The severity of the message. For example, `CL_MSG_WARNING`.
 * @param msg The message to be displayed.
 */
cl_error cl_abi_display_message(unsigned level, const char *msg);

/**
 * Signals to the frontend to write memory region data into the given array.
 * Cannot be stubbed.
 * @param regions A pointer to an array of memory regions to be filled in. The
 *   implementor is responsible for allocating this memory, and it will be
 *   freed by the caller when classicslive-integration deconstructs.
 * @param region_count A pointer to an unsigned integer that will be set to
 *   the number of memory regions written.
 */
cl_error cl_abi_install_memory_regions(cl_memory_region_t **regions,
                                       unsigned *region_count);

/**
 * Requests from the frontend the name of the target library or program.
 * This could be the name of a dynamically linked library, an external
 *   process name, or the name of a program classicslive-integration has
 *   been statically compiled into.
 * Cannot be stubbed.
 * @param name A pointer that will be set to the library name string.
 */
cl_error cl_abi_library_name(const char **name);

/**
 * Instructs the frontend to send an HTTP POST request.
 * Cannot be stubbed.
 * @param url The URL to send the POST request to.
 * @param data The data to be sent in the POST request. The caller is
 *   responsible for managing this pointer's memory.
 * @param callback A function to be called when a response is received.
 * @param userdata A pointer to user data that will be passed to the
 *   callback function. The callback function is responsible for
 *   managing this pointer's memory.
 * @warning For internal use. After implementing in the frontend, user
 *   code should call `cl_network_post` instead.
 */
cl_error cl_abi_network_post(const char *url, char *data,
                             cl_network_cb_t callback, void *userdata);

/**
 * Signals to the frontend to halt or resume the target program.
 * Can be stubbed, but not recommended if the editor is used.
 * @param mode The pause mode. Nonzero = pause, zero = resume.
 */
cl_error cl_abi_set_pause(unsigned mode);

/**
 * Instructs the frontend to spin a function into a seperate thread.
 * Cannot be stubbed.
 * @param task The task to be run in a new thread.
 */
cl_error cl_abi_thread(cl_task_t *task);

/**
 * Requests known user data from the frontend, used for retrieving
 *   login information and settings data.
 * Cannot be stubbed.
 * @param user A pointer to the user data being written to.
 * @param index The user index being requested. Zero-indexed.
 */
cl_error cl_abi_user_data(cl_user_t *user, unsigned index);

#if CL_EXTERNAL_MEMORY

/**
 * Instructs the frontend to copy external memory into a buffer.
 * Cannot be stubbed if `CL_EXTERNAL_MEMORY` is nonzero.
 * @param dest The destination buffer.
 * @param address The source virtual address.
 * @param size The number of bytes to copy.
 * @param read The number of bytes successfully read.
 */
cl_error cl_abi_external_read(void *dest, cl_addr_t address,
                              unsigned size, unsigned *read);

 /**
  * Instructs the frontend to copy data to external memory.
  * Cannot be stubbed if `CL_EXTERNAL_MEMORY` is nonzero.
  * @param src The source buffer.
  * @param address The destination virtual address.
  * @param size The number of bytes to copy.
  * @param written The number of bytes successfully written.
  */
cl_error cl_abi_external_write(const void *src, cl_addr_t address,
                               unsigned size, unsigned *written);

#endif

typedef struct
{
  unsigned version;

  struct functions
  {
    struct core
    {
      /** @see cl_abi_display_message */
      cl_error (*display_message)(unsigned level, const char *msg);

      /** @see cl_abi_install_memory_regions */
      cl_error (*install_memory_regions)(cl_memory_region_t **regions,
                                         unsigned *region_count);

      /** @see cl_abi_library_name */
      cl_error (*library_name)(const char **name);

      /** @see cl_abi_network_post */
      cl_error (*network_post)(const char *url, char *data,
                               cl_network_cb_t callback, void *userdata);

      /** @see cl_abi_set_pause */
      cl_error (*set_pause)(unsigned mode);

      /** @see cl_abi_thread */
      cl_error (*thread)(cl_task_t *task);

      /** @see cl_abi_user_data */
      cl_error (*user_data)(cl_user_t *user, unsigned index);
    } core;

    struct external
    {
      /** @see cl_abi_external_read */
      cl_error (*read)(void *dest, cl_addr_t address, unsigned size,
                       unsigned *read);

      /** @see cl_abi_external_write */
      cl_error (*write)(const void *src, cl_addr_t address, unsigned size,
                        unsigned *written);
    } external;
  } functions;
} cl_abi_t;

/**
 * Gives an ABI implementation to classicslive-integration. Must be called by
 *   the frontend before any classicslive-integration functions are used.
 * @param abi The frontend's ABI implementation
 */
cl_error cl_abi_register(const cl_abi_t *abi);

#ifdef __cplusplus
}
#endif

#endif
