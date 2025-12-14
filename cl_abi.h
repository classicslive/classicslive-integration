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

typedef struct
{
  unsigned version;

  struct functions
  {
    struct core
    {
#define CL_ABI_COMMENT_DISPLAY_MESSAGE \
      /** \
       * Signals to the frontend to display a message. \
       * Can be stubbed, but not recommended. \
       * @param level The severity of the message. For example, \
       * `CL_MSG_WARNING`. \
       * @param msg The message to be displayed. \
       */
      CL_ABI_COMMENT_DISPLAY_MESSAGE
      cl_error (*display_message)(unsigned level, const char *msg);

#define CL_ABI_COMMENT_INSTALL_MEMBANKS \
      /** \
       * Signals to the frontend to write memory bank data into the global \
       * "memory" struct. \
       * Cannot be stubbed. \
       */
      CL_ABI_COMMENT_INSTALL_MEMBANKS
      cl_error (*install_membanks)(void);

#define CL_ABI_COMMENT_LIBRARY_NAME \
      /** \
       * Has the frontend return a string representing the name of the \
       * program that is having its memory inspected. \
       * This could be the name of a dynamically linked library, an external \
       * process name, or the name of a program CL has been statically \
       * compiled into. \
       * Cannot be stubbed. \
       * @param name A pointer that will be set to the library name \
       */
      CL_ABI_COMMENT_LIBRARY_NAME
      cl_error (*library_name)(const char **name);

#define CL_ABI_COMMENT_NETWORK_POST \
      /** \
       * Instructs the frontend to send an HTTP POST request. \
       * Cannot be stubbed. \
       * @param url The URL to send the POST request to. \
       * @param data The data to be sent in the POST request. The caller is \
       *   responsible for managing this pointer's memory. \
       * @param callback A function to be called when a response is received. \
       * @param userdata A pointer to user data that will be passed to the \
       *   callback function. The callback function is responsible for \
       *   managing this pointer's memory. \
       * @warning For internal use. After implementing in the frontend, user \
       * code should call `cl_network_post` instead. \
       */
      CL_ABI_COMMENT_NETWORK_POST
      cl_error (*network_post)(const char *url, char *data,
                               cl_network_cb_t callback, void *userdata);

#define CL_ABI_COMMENT_SET_PAUSE \
      /** \
       * Signals to the frontend to halt or resume the target program. \
       * Can be stubbed, but not recommended if the editor is used. \
       * @param mode The pause mode. 1 = pause, 0 = resume. \
       */
      CL_ABI_COMMENT_SET_PAUSE
      cl_error (*set_pause)(unsigned mode);

#define CL_ABI_COMMENT_THREAD \
      /** \
       * Instructs the frontend to spin a function into a seperate thread. \
       * Cannot be stubbed. \
       * @param task The task to be run in a new thread. \
       */
      CL_ABI_COMMENT_THREAD
      cl_error (*thread)(cl_task_t *task);

#define CL_ABI_COMMENT_USER_DATA \
      /** \
       * Requests known user data from the frontend, used for retrieving \
       * login information and settings data. \
       * @param user A pointer to the user data being written to. \
       * @param index The user index being requested. Zero-indexed. \
       */
      CL_ABI_COMMENT_USER_DATA
      cl_error (*user_data)(cl_user_t *user, unsigned index);
    } core;

    struct external
    {
      /**
       * Instructs the frontend to copy external memory into a buffer.
       * Cannot be stubbed if `CL_EXTERNAL_MEMORY` is nonzero.
       * @param dest The destination buffer.
       * @param address The source virtual address.
       * @param size The number of bytes to copy.
       * @param read The number of bytes successfully read.
       */
      cl_error (*read)(void *dest, cl_addr_t address, unsigned size,
                       unsigned *read);

      /**
       * Instructs the frontend to copy data to external memory.
       * Cannot be stubbed if `CL_EXTERNAL_MEMORY` is nonzero.
       * @param src The source buffer.
       * @param address The destination virtual address.
       * @param size The number of bytes to copy.
       * @param written The number of bytes successfully written.
       */
      cl_error (*write)(const void *src, cl_addr_t address, unsigned size,
                        unsigned *written);
    } external;
  } functions;
} cl_abi_t;

/**
 * Gives an ABI implementation to classicslive-integration. Must be called by
 * the frontend before any classicslive-integration functions are used.
 * @param abi The frontend's ABI implementation
 */
cl_error cl_abi_register(const cl_abi_t *abi);

CL_ABI_COMMENT_DISPLAY_MESSAGE
cl_error cl_abi_display_message(unsigned level, const char *msg);

CL_ABI_COMMENT_INSTALL_MEMBANKS
cl_error cl_abi_install_membanks(void);

CL_ABI_COMMENT_LIBRARY_NAME
cl_error cl_abi_library_name(const char **name);

CL_ABI_COMMENT_NETWORK_POST
cl_error cl_abi_network_post(const char *url, char *data,
                             cl_network_cb_t callback, void *userdata);

CL_ABI_COMMENT_SET_PAUSE
cl_error cl_abi_set_pause(unsigned mode);

CL_ABI_COMMENT_THREAD
cl_error cl_abi_thread(cl_task_t *task);

CL_ABI_COMMENT_USER_DATA
cl_error cl_abi_user_data(cl_user_t *user, unsigned index);

#if CL_EXTERNAL_MEMORY
cl_error cl_abi_external_read(void *dest, cl_addr_t address,
                              unsigned size, unsigned *read);

cl_error cl_abi_external_write(const void *src, cl_addr_t address,
                               unsigned size, unsigned *written);
#endif

#ifdef __cplusplus
}
#endif

#endif
