#ifndef CL_CONFIG_H
#define CL_CONFIG_H

#define _CL_PLATFORM_UNKNOWN 0
#define _CL_PLATFORM_WINDOWS 1
#define _CL_PLATFORM_LINUX 2
#define _CL_PLATFORM_MACOS 3
#define _CL_PLATFORM_ANDROID 4
#define _CL_PLATFORM_NINTENDO_64 5
#define _CL_PLATFORM_GAMECUBE 6
#define _CL_PLATFORM_WII 7
#define _CL_PLATFORM_WII_U 8
#define _CL_PLATFORM_SWITCH 9
#define _CL_PLATFORM_SWITCH_2 10

typedef enum
{
  CL_PLATFORM_UNKNOWN = _CL_PLATFORM_UNKNOWN,

  CL_PLATFORM_WINDOWS = _CL_PLATFORM_WINDOWS,
  CL_PLATFORM_LINUX = _CL_PLATFORM_LINUX,
  CL_PLATFORM_MACOS = _CL_PLATFORM_MACOS,
  CL_PLATFORM_ANDROID = _CL_PLATFORM_ANDROID,
  CL_PLATFORM_NINTENDO_64 = _CL_PLATFORM_NINTENDO_64,
  CL_PLATFORM_GAMECUBE = _CL_PLATFORM_GAMECUBE,
  CL_PLATFORM_WII = _CL_PLATFORM_WII,
  CL_PLATFORM_WII_U = _CL_PLATFORM_WII_U,
  CL_PLATFORM_SWITCH = _CL_PLATFORM_SWITCH,
  CL_PLATFORM_SWITCH_2 = _CL_PLATFORM_SWITCH_2,

  CL_PLATFORM_SIZE
} cl_platform;

#define _CL_BITNESS_UNKNOWN 0
#define _CL_BITNESS_64 1
#define _CL_BITNESS_32 2

typedef enum
{
  CL_BITNESS_UNKNOWN = _CL_BITNESS_UNKNOWN,
  CL_BITNESS_64 = _CL_BITNESS_64,
  CL_BITNESS_32 = _CL_BITNESS_32,
  CL_BITNESS_SIZE
} cl_bitness;

#define _CL_ENDIANNESS_LITTLE 1
#define _CL_ENDIANNESS_BIG 2

#ifndef CL_HOST_PLATFORM
  #if defined(N64) || defined(__N64__) || defined(_N64)
    #define CL_HOST_PLATFORM _CL_PLATFORM_NINTENDO_64
  #elif defined(GEKKO)
    #if defined(WIIU)
      #define CL_HOST_PLATFORM _CL_PLATFORM_WII_U
    #elif defined(WII)
      #define CL_HOST_PLATFORM _CL_PLATFORM_WII
    #else
      #define CL_HOST_PLATFORM _CL_PLATFORM_GAMECUBE
    #endif
  #elif defined(WIN32) || defined(_WIN32)
    #define CL_HOST_PLATFORM _CL_PLATFORM_WINDOWS
    #if defined(WIN64) || defined(_WIN64)
      #define CL_HOST_BITNESS _CL_BITNESS_64
    #else
      #define CL_HOST_BITNESS _CL_BITNESS_32
    #endif
  #elif defined(__linux__) && !defined(__ANDROID__)
    #define CL_HOST_PLATFORM _CL_PLATFORM_LINUX
    #if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
      #define CL_HOST_BITNESS _CL_BITNESS_64
    #else
      #define CL_HOST_BITNESS _CL_BITNESS_32
    #endif
  #elif defined(__APPLE__) && defined(__MACH__)
    #define CL_HOST_PLATFORM _CL_PLATFORM_MACOS
    #if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
      #define CL_HOST_BITNESS _CL_BITNESS_64
    #else
      #define CL_HOST_BITNESS _CL_BITNESS_32
    #endif
  #elif defined(__ANDROID__)
    #define CL_HOST_PLATFORM _CL_PLATFORM_ANDROID
    #if defined(__x86_64__) || defined(__aarch64__)
      #define CL_HOST_BITNESS _CL_BITNESS_64
    #else
      #define CL_HOST_BITNESS _CL_BITNESS_32
    #endif
  #else
    #error "Unable to determine host platform"
  #endif
#endif

#if CL_HOST_PLATFORM == _CL_PLATFORM_NINTENDO_64 || \
    CL_HOST_PLATFORM == _CL_PLATFORM_GAMECUBE || \
    CL_HOST_PLATFORM == _CL_PLATFORM_WII || \
    CL_HOST_PLATFORM == _CL_PLATFORM_WII_U
  #define CL_HOST_BITNESS _CL_BITNESS_32
  #define CL_HOST_ENDIANNESS _CL_ENDIANNESS_BIG
#endif

#ifndef CL_HOST_BITNESS
  #error "Unable to determine host bitness"
#endif

#ifndef CL_HOST_ENDIANNESS
  #if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define CL_HOST_ENDIANNESS _CL_ENDIANNESS_BIG
  #elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define CL_HOST_ENDIANNESS _CL_ENDIANNESS_LITTLE
  #else
    #warning "Unable to determine host endianness, assuming little-endian"
    #define CL_HOST_ENDIANNESS _CL_ENDIANNESS_LITTLE
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
#define CL_URL_HOSTNAME "192.168.86.15:8000"
#endif

typedef struct cl_config_t
{
  char url[256];
} cl_config_t;

cl_config_t cl_default_config(void);

#endif
