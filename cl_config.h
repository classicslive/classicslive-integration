#ifndef CL_CONFIG_H
#define CL_CONFIG_H

#define CL_PLATFORM_UNKNOWN 0

#define CL_PLATFORM_WINDOWS 1
#define CL_PLATFORM_LINUX 2
#define CL_PLATFORM_MACOS 3
#define CL_PLATFORM_ANDROID 4
#define CL_PLATFORM_WII_U 5

#define CL_PLATFORM_64 1
#define CL_PLATFORM_32 2

#ifndef CL_HOST_PLATFORM
  #if defined(WIN32) || defined(_WIN32)
    #define CL_HOST_PLATFORM CL_PLATFORM_WINDOWS
    #if defined(WIN64) || defined(_WIN64)
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_64
    #else
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_32
    #endif
  #elif defined(__linux__) && !defined(__ANDROID__)
    #define CL_HOST_PLATFORM CL_PLATFORM_LINUX
    #if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_64
    #else
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_32
    #endif
  #elif defined(__APPLE__) && defined(__MACH__)
    #define CL_HOST_PLATFORM CL_PLATFORM_MACOS
    #if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_64
    #else
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_32
    #endif
  #elif defined(__ANDROID__)
    #define CL_HOST_PLATFORM CL_PLATFORM_ANDROID
    #if defined(__x86_64__) || defined(__aarch64__)
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_64
    #else
      #define CL_HOST_ARCHITECTURE CL_PLATFORM_32
    #endif
  #elif defined(__WIIU__)
    #define CL_HOST_PLATFORM CL_PLATFORM_WII_U
    #define CL_HOST_ARCHITECTURE CL_PLATFORM_32
  #else
    #define CL_HOST_PLATFORM CL_PLATFORM_UNKNOWN
    #define CL_HOST_ARCHITECTURE CL_PLATFORM_UNKNOWN
    #error "Unrecognized target platform"
  #endif
#endif

#ifndef CL_HAVE_EDITOR
/**
 * Whether or not the Classics Live Editor is included in this implementation.
 * The Classics Live Editor requires a C++ compiler and Qt5.
 */
#define CL_HAVE_EDITOR false
#endif

#ifndef CL_HAVE_FILESYSTEM
/**
 * Whether or not the filesystem will be accessed in this implementation.
 * If false, only raw data passed in cl_init can be used for indentification.
 */
#define CL_HAVE_FILESYSTEM false
#endif

#ifndef CL_HAVE_SSL
/**
 * Whether or not the networking callbacks in this implementation support HTTPS.
 * This should be assumed true and only changed with caution.
 */
#define CL_HAVE_SSL true
#endif

#ifndef CL_EXTERNAL_MEMORY
/**
 * Whether or not the target memory is external to this program, ie. being read
 * from another process.
 * If true, the frontend needs to supply implementations of cl_fe_memory_read
 * and cl_fe_memory_write. See cl_frontend.h.
 */
#define CL_EXTERNAL_MEMORY false
#endif

#ifndef CL_LIBRETRO
/**
 * Whether or not this implementation is a libretro frontend.
 */
#define CL_LIBRETRO false
#endif

#ifndef CL_URL_HOSTNAME
/**
 * The full hostname for the CL website.
 */
#define CL_URL_HOSTNAME "classicslive.doggylongface.com"
#endif

typedef struct cl_config_t
{
  char url[256];
} cl_config_t;

cl_config_t cl_default_config(void);

#endif
