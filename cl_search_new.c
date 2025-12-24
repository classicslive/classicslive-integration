#include "cl_search_new.h"

#include "cl_config.h"
#include "cl_memory.h"

#if CL_HOST_PLATFORM == _CL_PLATFORM_LINUX && 0
  #include <sys/mman.h>
#elif CL_HOST_PLATFORM == _CL_PLATFORM_WINDOWS && 0
  #include <windows.h>
#endif

#include <stdint.h>
#include <string.h>

typedef union
{
  uint8_t u8;
  int8_t s8;
  uint16_t u16;
  int16_t s16;
  uint32_t u32;
  int32_t s32;
  int64_t s64;
  float fp;
  double dfp;
} cl_search_target_impl_t;

#define CL_TARGET(target) ((cl_search_target_impl_t *)&(target))

/**
 * Allocate a chunk of page-aligned memory.
 * @param size The number of bytes to allocate
 * @return A pointer to the bytes, or NULL
 */
static void *cl_mmap(size_t size)
{
#if CL_HOST_PLATFORM == _CL_PLATFORM_LINUX && 0
  void *p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (p == CL_ADDRESS_INVALID)
    return NULL;
  else
    return p;
#elif CL_HOST_PLATFORM == _CL_PLATFORM_WINDOWS && 0
  return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  return malloc(size);
#endif
}

/**
 * Free a chunk of page-aligned memory.
 * @param p The pointer to free
 * @param size The number of bytes to deallocate
 */
static void cl_munmap(void *p, size_t size)
{
#if CL_HOST_PLATFORM == _CL_PLATFORM_LINUX && 0
  munmap(p, size);
#elif CL_HOST_PLATFORM == _CL_PLATFORM_WINDOWS && 0
  CL_UNUSED(size);
  VirtualFree(p, 0, MEM_RELEASE);
#else
  CL_UNUSED(size);
  free(p);
#endif
}

#define CL_PASTE2(a, b) a##b
#define CL_PASTE3(a, b, c) a##b##c
#define CL_PASTE4(a, b, c, d) a##b##c##d

#if defined(_MSC_VER)
#include <intrin.h>
#define CL_SEARCH_SWAP_s8(a) (a)
#define CL_SEARCH_SWAP_u8(a) (a)
#define CL_SEARCH_SWAP_s16(a) _byteswap_ushort(*(uint16_t*)&(a))
#define CL_SEARCH_SWAP_u16(a) _byteswap_ushort(*(uint16_t*)&(a))
#define CL_SEARCH_SWAP_s32(a) _byteswap_ulong(*(uint32_t*)&(a))
#define CL_SEARCH_SWAP_u32(a) _byteswap_ulong(*(uint32_t*)&(a))
#define CL_SEARCH_SWAP_s64(a) _byteswap_uint64(*(uint64_t*)&(a))
#elif defined(__GNUC__) || defined(__clang__)
#define CL_SEARCH_SWAP_s8(a) (a)
#define CL_SEARCH_SWAP_u8(a) (a)
#define CL_SEARCH_SWAP_s16(a) __builtin_bswap16(*(uint16_t*)&(a))
#define CL_SEARCH_SWAP_u16(a) __builtin_bswap16(*(uint16_t*)&(a))
#define CL_SEARCH_SWAP_s32(a) __builtin_bswap32(*(uint32_t*)&(a))
#define CL_SEARCH_SWAP_u32(a) __builtin_bswap32(*(uint32_t*)&(a))
#define CL_SEARCH_SWAP_s64(a) __builtin_bswap64(*(uint64_t*)&(a))
#else
#error "Endianness helpers are not supported on this compiler. You should try compiling with CL_HAVE_SEARCH=0."
#endif

static float cl_bswap_float(float f)
{
  union { uint32_t i; float f; } u;
  u.f = f;
  u.i = CL_SEARCH_SWAP_u32(u.i);
  return u.f;
}

static double cl_bswap_double(double d)
{
  union { uint64_t i; double d; } u;
  u.d = d;
  u.i = CL_SEARCH_SWAP_s64(u.i);
  return u.d;
}

#define CL_SEARCH_SWAP_fp(a) cl_bswap_float(a)
#define CL_SEARCH_SWAP_dfp(a) cl_bswap_double(a)

/**
 * Kernel builder to compare an immediate to native-endian guest memory.
 */
#define CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE3(cl_search_cmp_imm_, b, _##d)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a right = ((cl_search_target_impl_t*)(target))->b; \
  CL_UNUSED(chunk_data_prev); \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (*chunk_data_cast c right) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
  } \
  return matches; \
}

/**
 * Kernel builder to compare an immediate to opposite-endian guest memory,
 * byteswapping the immediate once. This works only for equality/inequality,
 * but is faster.
 */
#define CL_SEARCH_CMP_IMMEDIATE_SWAPHOST_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE4(cl_search_cmp_imm_, b, _##d, _swaphost)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a right = CL_SEARCH_SWAP_##b(((cl_search_target_impl_t*)(target))->b); \
  CL_UNUSED(chunk_data_prev); \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (*chunk_data_cast c right) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
  } \
  return matches; \
}

/**
 * Kernel builder to compare an immediate to opposite-endian guest memory,
 * byteswapping each guest value in sequence. This works for all other forms
 * of comparison.
 */
#define CL_SEARCH_CMP_IMMEDIATE_SWAPGUEST_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE4(cl_search_cmp_imm_, b, _##d, _swapguest)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a right = ((cl_search_target_impl_t *)(target))->b; \
  CL_UNUSED(chunk_data_prev); \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = ((a)CL_SEARCH_SWAP_##b(*chunk_data_cast) c (a)right) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
  } \
  return matches; \
}

/**
 * Kernel builder to compare each value to its value from the previous search
 * step, in native-endian.
 */
#define CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE3(cl_search_cmp_prv_, b, _##d)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a *chunk_data_prev_cast = (const a*)chunk_data_prev; \
  CL_UNUSED(target); \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (*chunk_data_cast c *chunk_data_prev_cast) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
    chunk_data_prev_cast++; \
  } \
  return matches; \
}

/**
 * Kernel builder to compare each value to its value from the previous search
 * step, swapping endianness of every value.
 */
#define CL_SEARCH_CMP_PREVIOUS_SWAPBOTH_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE4(cl_search_cmp_prv_, b, _##d, _swapboth)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *chunk_data_cast = (a*)chunk_data; \
  const a *chunk_data_end_cast = (const a*)chunk_data_end; \
  const a *chunk_data_prev_cast = (const a*)chunk_data_prev; \
  CL_UNUSED(target); \
  while (chunk_data_cast < chunk_data_end_cast) \
  { \
    match = (CL_SEARCH_SWAP_##b(*chunk_data_cast) c \
             CL_SEARCH_SWAP_##b(*chunk_data_prev_cast)) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    chunk_data_cast++; \
    chunk_validity++; \
    chunk_data_prev_cast++; \
  } \
  return matches; \
}

#define CL_SEARCH_CMP_DELTA_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE3(cl_search_cmp_dlt_, b, _##d)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *cur = (a*)chunk_data; \
  const a *end = (const a*)chunk_data_end; \
  const a *prev = (const a*)chunk_data_prev; \
  const a delta = ((const cl_search_target_impl_t*)(target))->b; \
  while (cur < end) \
  { \
    match = ((*cur) == (*prev c delta)) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    cur++; \
    prev++; \
    chunk_validity++; \
  } \
  return matches; \
}

#define CL_SEARCH_CMP_DELTA_SWAPBOTH_TEMPLATE(a, b, c, d) \
static unsigned CL_PASTE4(cl_search_cmp_dlt_, b, _##d, _swapboth)( \
  void *chunk_data, \
  const void *chunk_data_end, \
  unsigned char *chunk_validity, \
  const void *chunk_data_prev, \
  const void *target) \
{ \
  unsigned matches = 0; \
  unsigned char match; \
  a *cur = (a*)chunk_data; \
  const a *end = (const a*)chunk_data_end; \
  const a *prev = (const a*)chunk_data_prev; \
  const a delta = ((const cl_search_target_impl_t*)(target))->b; \
  while (cur < end) \
  { \
    match = (((a)CL_SEARCH_SWAP_##b(*cur) == \
             ((a)CL_SEARCH_SWAP_##b(*prev) c delta))) & *chunk_validity; \
    *chunk_validity = match; \
    matches += match; \
    cur++; \
    prev++; \
    chunk_validity++; \
  } \
  return matches; \
}

/** Unroll kernels with endianness ignored */
#define CL_SEARCH_CMP_IMMEDIATE_UNROLL_8BIT(a, b) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, !=, neq) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, !=, neq) \
  CL_SEARCH_CMP_DELTA_TEMPLATE(a, b, +, inc) \
  CL_SEARCH_CMP_DELTA_TEMPLATE(a, b, -, dec) \
  \

/** Unroll kernels with endianness accounted for */
#define CL_SEARCH_CMP_IMMEDIATE_UNROLL(a, b) \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_IMMEDIATE_SWAPHOST_TEMPLATE(a, b, ==, equ) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, ==, equ) \
  \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, !=, neq) \
  CL_SEARCH_CMP_IMMEDIATE_SWAPHOST_TEMPLATE(a, b, !=, neq) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, !=, neq) \
  \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_IMMEDIATE_SWAPGUEST_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, <, les) \
  CL_SEARCH_CMP_PREVIOUS_SWAPBOTH_TEMPLATE(a, b, <, les) \
  \
  CL_SEARCH_CMP_IMMEDIATE_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_IMMEDIATE_SWAPGUEST_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_PREVIOUS_TEMPLATE(a, b, >, gtr) \
  CL_SEARCH_CMP_PREVIOUS_SWAPBOTH_TEMPLATE(a, b, >, gtr) \
  \
  CL_SEARCH_CMP_DELTA_TEMPLATE(a, b, +, inc) \
  CL_SEARCH_CMP_DELTA_SWAPBOTH_TEMPLATE(a, b, +, inc) \
  \
  CL_SEARCH_CMP_DELTA_TEMPLATE(a, b, -, dec) \
  CL_SEARCH_CMP_DELTA_SWAPBOTH_TEMPLATE(a, b, -, dec) \
  \

CL_SEARCH_CMP_IMMEDIATE_UNROLL_8BIT(uint8_t, u8)
CL_SEARCH_CMP_IMMEDIATE_UNROLL_8BIT(int8_t, s8)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(uint16_t, u16)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(int16_t, s16)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(uint32_t, u32)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(int32_t, s32)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(int64_t, s64)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(float, fp)
CL_SEARCH_CMP_IMMEDIATE_UNROLL(double, dfp)

typedef unsigned (*cl_search_compare_func_t)(void*,const void*,unsigned char*,const void*,const void*);

static cl_search_compare_func_t cl_search_comparison_function(
  cl_search_parameters_t params, cl_endianness endianness)
{
  int swap = (endianness != CL_HOST_ENDIANNESS);

  switch (params.value_type)
  {
  case CL_MEMTYPE_UINT8:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u8_equ
        : cl_search_cmp_imm_u8_equ;
    case CL_COMPARE_GREATER:
      return params.target_none
        ? cl_search_cmp_prv_u8_gtr
        : cl_search_cmp_imm_u8_gtr;
    case CL_COMPARE_LESS:
      return params.target_none
        ? cl_search_cmp_prv_u8_les
        : cl_search_cmp_imm_u8_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u8_neq
        : cl_search_cmp_imm_u8_neq;
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? cl_search_cmp_prv_u8_gtr
        : cl_search_cmp_dlt_u8_inc;
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? cl_search_cmp_prv_u8_les
        : cl_search_cmp_dlt_u8_dec;
    default:
      return NULL;
    }

  case CL_MEMTYPE_INT8:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s8_equ
        : cl_search_cmp_imm_s8_equ;
    case CL_COMPARE_GREATER:
      return params.target_none
        ? cl_search_cmp_prv_s8_gtr
        : cl_search_cmp_imm_s8_gtr;
    case CL_COMPARE_LESS:
      return params.target_none
        ? cl_search_cmp_prv_s8_les
        : cl_search_cmp_imm_s8_les;
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s8_neq
        : cl_search_cmp_imm_s8_neq;
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? cl_search_cmp_prv_s8_gtr
        : cl_search_cmp_dlt_s8_inc;
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? cl_search_cmp_prv_s8_les
        : cl_search_cmp_dlt_s8_dec;
    default:
      return NULL;
    }

  case CL_MEMTYPE_UINT16:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u16_equ
        : (swap ? cl_search_cmp_imm_u16_equ_swaphost :
                  cl_search_cmp_imm_u16_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u16_gtr_swapboth :
                  cl_search_cmp_prv_u16_gtr)
        : (swap ? cl_search_cmp_imm_u16_gtr_swapguest :
                  cl_search_cmp_imm_u16_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u16_les_swapboth :
                  cl_search_cmp_prv_u16_les)
        : (swap ? cl_search_cmp_imm_u16_les_swapguest :
                  cl_search_cmp_imm_u16_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u16_neq
        : (swap ? cl_search_cmp_imm_u16_neq_swaphost :
                  cl_search_cmp_imm_u16_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u16_gtr_swapboth :
                  cl_search_cmp_prv_u16_gtr)
        : (swap ? cl_search_cmp_dlt_u16_inc_swapboth :
                  cl_search_cmp_dlt_u16_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u16_les_swapboth :
                  cl_search_cmp_prv_u16_les)
        : (swap ? cl_search_cmp_dlt_u16_dec_swapboth :
                  cl_search_cmp_dlt_u16_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_INT16:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s16_equ
        : (swap ? cl_search_cmp_imm_s16_equ_swaphost :
                  cl_search_cmp_imm_s16_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s16_gtr_swapboth :
                  cl_search_cmp_prv_s16_gtr)
        : (swap ? cl_search_cmp_imm_s16_gtr_swapguest :
                  cl_search_cmp_imm_s16_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s16_les_swapboth :
                  cl_search_cmp_prv_s16_les)
        : (swap ? cl_search_cmp_imm_s16_les_swapguest :
                  cl_search_cmp_imm_s16_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s16_neq
        : (swap ? cl_search_cmp_imm_s16_neq_swaphost :
                  cl_search_cmp_imm_s16_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s16_gtr_swapboth :
                  cl_search_cmp_prv_s16_gtr)
        : (swap ? cl_search_cmp_dlt_s16_inc_swapboth :
                  cl_search_cmp_dlt_s16_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s16_les_swapboth :
                  cl_search_cmp_prv_s16_les)
        : (swap ? cl_search_cmp_dlt_s16_dec_swapboth :
                  cl_search_cmp_dlt_s16_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_UINT32:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u32_equ
        : (swap ? cl_search_cmp_imm_u32_equ_swaphost :
                  cl_search_cmp_imm_u32_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u32_gtr_swapboth :  
                  cl_search_cmp_prv_u32_gtr)
        : (swap ? cl_search_cmp_imm_u32_gtr_swapguest :
                  cl_search_cmp_imm_u32_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u32_les_swapboth :
                  cl_search_cmp_prv_u32_les)
        : (swap ? cl_search_cmp_imm_u32_les_swapguest :
                  cl_search_cmp_imm_u32_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_u32_neq
        : (swap ? cl_search_cmp_imm_u32_neq_swaphost :
                  cl_search_cmp_imm_u32_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u32_gtr_swapboth :
                  cl_search_cmp_prv_u32_gtr)
        : (swap ? cl_search_cmp_dlt_u32_inc_swapboth :
                  cl_search_cmp_dlt_u32_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_u32_les_swapboth :
                  cl_search_cmp_prv_u32_les)
        : (swap ? cl_search_cmp_dlt_u32_dec_swapboth :
                  cl_search_cmp_dlt_u32_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_INT32:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s32_equ
        : (swap ? cl_search_cmp_imm_s32_equ_swaphost :
                  cl_search_cmp_imm_s32_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s32_gtr_swapboth :
                  cl_search_cmp_prv_s32_gtr)
        : (swap ? cl_search_cmp_imm_s32_gtr_swapguest :
                  cl_search_cmp_imm_s32_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s32_les_swapboth :
                  cl_search_cmp_prv_s32_les)
        : (swap ? cl_search_cmp_imm_s32_les_swapguest :
                  cl_search_cmp_imm_s32_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s32_neq
        : (swap ? cl_search_cmp_imm_s32_neq_swaphost :
                  cl_search_cmp_imm_s32_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s32_gtr_swapboth :
                  cl_search_cmp_prv_s32_gtr)
        : (swap ? cl_search_cmp_dlt_s32_inc_swapboth :
                  cl_search_cmp_dlt_s32_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s32_les_swapboth :
                  cl_search_cmp_prv_s32_les)
        : (swap ? cl_search_cmp_dlt_s32_dec_swapboth :
                  cl_search_cmp_dlt_s32_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_INT64:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s64_equ
        : (swap ? cl_search_cmp_imm_s64_equ_swaphost :
                  cl_search_cmp_imm_s64_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s64_gtr_swapboth :
                  cl_search_cmp_prv_s64_gtr)
        : (swap ? cl_search_cmp_imm_s64_gtr_swapguest :
                  cl_search_cmp_imm_s64_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s64_les_swapboth :
                  cl_search_cmp_prv_s64_les)
        : (swap ? cl_search_cmp_imm_s64_les_swapguest :
                  cl_search_cmp_imm_s64_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_s64_neq
        : (swap ? cl_search_cmp_imm_s64_neq_swaphost :
                  cl_search_cmp_imm_s64_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s64_gtr_swapboth :
                  cl_search_cmp_prv_s64_gtr)
        : (swap ? cl_search_cmp_dlt_s64_inc_swapboth :
                  cl_search_cmp_dlt_s64_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_s64_les_swapboth :
                  cl_search_cmp_prv_s64_les)
        : (swap ? cl_search_cmp_dlt_s64_dec_swapboth :
                  cl_search_cmp_dlt_s64_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_FLOAT:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_fp_equ
        : (swap ? cl_search_cmp_imm_fp_equ_swaphost :
                  cl_search_cmp_imm_fp_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_fp_gtr_swapboth :
                  cl_search_cmp_prv_fp_gtr)
        : (swap ? cl_search_cmp_imm_fp_gtr_swapguest :
                  cl_search_cmp_imm_fp_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_fp_les_swapboth :
                  cl_search_cmp_prv_fp_les)
        : (swap ? cl_search_cmp_imm_fp_les_swapguest :
                  cl_search_cmp_imm_fp_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_fp_neq
        : (swap ? cl_search_cmp_imm_fp_neq_swaphost :
                  cl_search_cmp_imm_fp_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_fp_gtr_swapboth :
                  cl_search_cmp_prv_fp_gtr)
        : (swap ? cl_search_cmp_dlt_fp_inc_swapboth :
                  cl_search_cmp_dlt_fp_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_fp_les_swapboth :
                  cl_search_cmp_prv_fp_les)
        : (swap ? cl_search_cmp_dlt_fp_dec_swapboth :
                  cl_search_cmp_dlt_fp_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_DOUBLE:
    switch (params.compare_type)
    {
    case CL_COMPARE_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_dfp_equ
        : (swap ? cl_search_cmp_imm_dfp_equ_swaphost :
                  cl_search_cmp_imm_dfp_equ);
    case CL_COMPARE_GREATER:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_dfp_gtr_swapboth :
                  cl_search_cmp_prv_dfp_gtr)
        : (swap ? cl_search_cmp_imm_dfp_gtr_swapguest :
                  cl_search_cmp_imm_dfp_gtr);
    case CL_COMPARE_LESS:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_dfp_les_swapboth :
                  cl_search_cmp_prv_dfp_les)
        : (swap ? cl_search_cmp_imm_dfp_les_swapguest :
                  cl_search_cmp_imm_dfp_les);
    case CL_COMPARE_NOT_EQUAL:
      return params.target_none
        ? cl_search_cmp_prv_dfp_neq
        : (swap ? cl_search_cmp_imm_dfp_neq_swaphost :
                  cl_search_cmp_imm_dfp_neq);
    case CL_COMPARE_INCREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_dfp_gtr_swapboth :
                  cl_search_cmp_prv_dfp_gtr)
        : (swap ? cl_search_cmp_dlt_dfp_inc_swapboth :
                  cl_search_cmp_dlt_dfp_inc);
    case CL_COMPARE_DECREASED:
      return params.target_none
        ? (swap ? cl_search_cmp_prv_dfp_les_swapboth :
                  cl_search_cmp_prv_dfp_les)
        : (swap ? cl_search_cmp_dlt_dfp_dec_swapboth :
                  cl_search_cmp_dlt_dfp_dec);
    default:
      return NULL;
    }

  case CL_MEMTYPE_NOT_SET:
  case CL_MEMTYPE_SIZE:
    return NULL;
  }

  return NULL;
}


/**
 * Runs a comparison function on the values in a search page.
 * @param page
 */
static cl_error cl_search_step_page(cl_search_page_t *page,
  const cl_search_parameters_t params, cl_search_compare_func_t function,
  const void *prev_buffer)
{
  const void *end = (((unsigned char*)page->chunk) + page->size);

  if (!function)
    return CL_ERR_PARAMETER_INVALID;
  page->matches = function(page->chunk,
                           end,
                           page->validity,
                           prev_buffer,
                           &params.target);

  return CL_OK;
}

/**
 * Steps through the linked list to count up the total memory usage of a
 * search, storing the result in `search->memory_usage` as bytes.
 */
static cl_error cl_search_profile_memory(cl_search_t *search)
{
  cl_addr_t usage = 0;
  unsigned i;

  for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_t *page = search->page_regions[i].first_page;

    while (page)
    {
      /* Count the chunks */
      usage += page->size + page->size / search->params.value_size;
      usage += sizeof(cl_search_page_t);
      page = page->next;
    }
    usage += sizeof(cl_search_page_region_t);
  }
  usage += sizeof(cl_search_t);
  search->memory_usage = usage;

  return CL_OK;
}

cl_error cl_search_change_compare_type(cl_search_t *search,
  cl_compare_type compare_type)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (compare_type == CL_COMPARE_INVALID ||
           compare_type >= CL_COMPARE_SIZE)
    return CL_ERR_PARAMETER_INVALID;
  else
  {
    search->params.compare_type = compare_type;

    return CL_OK;
  }
}

/**
 * Return the canonical integer target value of a search.
 * @param search A pointer to the search to get the target from
 * @param out_target A pointer to store the target value into
 */
static cl_error cl_search_get_target_int(const cl_search_t *search, int64_t *out)
{
  if (!search || !out)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_target_impl_t *target = CL_TARGET(search->params.target);

    switch (search->params.value_size)
    {
    case 1:
      *out = target->s8;
      return CL_OK;
    case 2:
      *out = target->s16;
      return CL_OK;
    case 4:
      *out = target->s32;
      return CL_OK;
    case 8:
      *out = target->s64;
      return CL_OK;
    default:
      return CL_ERR_PARAMETER_INVALID;
    }
  }
}

/**
 * Return the canonical floating-point target value of a search.
 * @param search A pointer to the search to get the target from
 * @param out_target A pointer to store the target value into
 */
static cl_error cl_search_get_target_float(const cl_search_t *search, double *out)
{
  if (!search || !out)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_target_impl_t *target = CL_TARGET(search->params.target);

    switch (search->params.value_size)
    {
    case 4:
      *out = target->fp;
      return CL_OK;
    case 8:
      *out = target->dfp;
      return CL_OK;
    default:
      return CL_ERR_PARAMETER_INVALID;
    }
  }
}

static cl_error cl_search_set_target(cl_search_t *search, int64_t value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_target_impl_t *target = CL_TARGET(search->params.target);

    target->s64 = 0;
    switch (search->params.value_size)
    {
    case 1:
      target->s8 = (int8_t)value;
      break;
    case 2:
      target->s16 = (int16_t)value;
      break;
    case 4:
      target->s32 = (int32_t)value;
      break;
    case 8:
      target->s64 = (int64_t)value;
      break;
    default:
      return CL_ERR_PARAMETER_INVALID;
    }

    return CL_OK;
  }
}

cl_error cl_search_change_value_type(cl_search_t *search, cl_value_type type)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (type == CL_MEMTYPE_NOT_SET || type >= CL_MEMTYPE_SIZE)
    return CL_ERR_PARAMETER_INVALID;
  else
  {
    int64_t target_value = 0;

    cl_search_get_target_int(search, &target_value);
    search->params.value_type = type;
    search->params.value_size = cl_sizeof_memtype(type);
    cl_search_set_target(search, target_value);

    return CL_OK;
  }
}

cl_error cl_search_change_target(cl_search_t *search, const void *value)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (!value)
    search->params.target_none = 1;
  else
  {
    cl_search_target_t target;
    cl_search_target_impl_t *target_impl = CL_TARGET(target);

    target_impl->s64 = 0;
    switch (search->params.value_size)
    {
    case 1:
      target_impl->s8 = *(const int8_t*)value;
      break;
    case 2:
      target_impl->s16 = *(const int16_t*)value;
      break;
    case 4:
      target_impl->s32 = *(const int32_t*)value;
      break;
    case 8:
      target_impl->s64 = *(const int64_t*)value;
      break;
    default:
      return CL_ERR_PARAMETER_INVALID;
    }
    search->params.target = target;
    search->params.target_none = 0;
  }

  return CL_OK;
}

static cl_error cl_search_free_page(const cl_search_t *search,
  cl_search_page_t *page)
{
  if (page)
  {
    cl_munmap(page->chunk, page->size + page->size / search->params.value_size);
    free(page);

    return CL_OK;
  }

  return CL_ERR_PARAMETER_NULL;
}

cl_error cl_search_free(cl_search_t *search)
{
  unsigned i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];
    cl_search_page_t *page = page_region->first_page;
    cl_search_page_t *next_page = NULL;

    while (page)
    {
      next_page = page->next;
      cl_search_free_page(search, page);
      page = next_page;
    }
  }
  free(search->page_regions);
  memset(search, 0, sizeof(cl_search_t));

  return CL_OK;
}

cl_error cl_search_init(cl_search_t *search)
{
  unsigned i;

  if (!search || !memory.regions)
    return CL_ERR_PARAMETER_NULL;
  else if (memory.region_count == 0)
    return CL_ERR_PARAMETER_INVALID;
  else if (sizeof(cl_search_target_impl_t) != sizeof(cl_search_target_t))
  {
    cl_message(CL_MSG_ERROR, "Compile: search target impl size mismatch");
    return CL_ERR_CLIENT_COMPILE;
  }

  /* Zero-init the search */
  memset(search, 0, sizeof(cl_search_t));

  /* Allocate and init page regions */
  search->page_regions = (cl_search_page_region_t*)calloc(
    memory.region_count, sizeof(cl_search_page_region_t));
  search->page_region_count = memory.region_count;
  for (i = 0; i < memory.region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];

    page_region->page_count = 0;
    page_region->region = &memory.regions[i];
  }

  return cl_search_profile_memory(search);
}

static cl_error cl_search_step_print(const cl_search_t *search)
{
  cl_log("==============================\n");
  cl_log("Search step %u:\n", search->steps);
  cl_log("Find all %s %s ",
    cl_string_value_type(search->params.value_type),
    cl_string_compare_type(search->params.compare_type));
  if (search->params.target_none)
    cl_log("%s", (search->params.compare_type == CL_COMPARE_INCREASED ||
                  search->params.compare_type == CL_COMPARE_DECREASED) ?
                    "by any amount.\n" : "previous value.\n");
  else if (search->params.value_type == CL_MEMTYPE_FLOAT ||
           search->params.value_type == CL_MEMTYPE_DOUBLE)
  {
    double target_value = 0.0;

    cl_search_get_target_float(search, &target_value);
    cl_log("target value %f.\n", target_value);
  }
  else
  {
    int64_t target_value = 0;

    cl_search_get_target_int(search, &target_value);
    cl_log("target value %li.\n", target_value);
  }
  cl_log("------------------------------\n");
  cl_log("Total memory scanned: %.6f MB\n",
    ((double)search->memory_scanned) / (1024.0 * 1024.0));
  cl_log("Total matched addresses: %llu\n", (unsigned long long)search->total_matches);
  cl_log("Total pages allocated: %llu\n", (unsigned long long)search->total_page_count);
  cl_log("Time taken: %.6f seconds\n", search->time_taken);
  cl_log("Estimated memory usage: %.6f MB\n",
    ((double)search->memory_usage) / (1024.0 * 1024.0));

  return CL_OK;
}

/**
 * Performs the first search step, which is responsible for allocating the
 * initial round of chunks.
 * @param search A pointer to the search to perform a step on
 */
static cl_error cl_search_step_first(cl_search_t *search)
{
#if CL_EXTERNAL_MEMORY
  void *bucket = NULL;
  cl_addr_t bucket_offset = 0;
  cl_addr_t bucket_processed = 0;
  cl_addr_t bucket_size = 0;
#endif
  cl_search_compare_func_t function;
  clock_t start = clock();
  unsigned i;

#if CL_EXTERNAL_MEMORY
  bucket = cl_mmap(CL_SEARCH_BUCKET_SIZE);
  if (!bucket)
    return CL_ERR_CLIENT_RUNTIME;
#endif

  for (i = 0; i < search->page_region_count; i++)
  {
    cl_search_page_region_t *page_region = &search->page_regions[i];
    cl_search_page_t *page = NULL;
    cl_search_page_t *prev_page = NULL;
    cl_addr_t processed = 0;

    function = cl_search_comparison_function(search->params, page_region->region->endianness);
    if (!function)
      continue;

#if CL_EXTERNAL_MEMORY
    bucket_offset = 0;
    bucket_processed = 0;
    bucket_size = page_region->region->size < CL_SEARCH_BUCKET_SIZE ?
                  page_region->region->size : CL_SEARCH_BUCKET_SIZE;
    cl_read_memory_buffer(bucket, page_region->region, bucket_offset, bucket_size);
#endif

    while (processed < page_region->region->size)
    {
      unsigned size = CL_SEARCH_CHUNK_SIZE;

      if (processed + CL_SEARCH_CHUNK_SIZE > page_region->region->size)
        size = (unsigned)(page_region->region->size - processed);

      if (!page)
      {
        page = (cl_search_page_t*)calloc(1, sizeof(cl_search_page_t));
        page->chunk = cl_mmap(size + size / search->params.value_size);
        page->validity = (void*)((unsigned char*)page->chunk + size);
      }

      page->region = page_region->region;
      page->start = page_region->region->base_guest + processed;
      page->size = size;

#if CL_EXTERNAL_MEMORY
      memcpy(page->chunk, (unsigned char*)bucket + bucket_processed, page->size);
#else
      cl_read_memory_buffer(page->chunk, page->region,
        page->start - page->region->base_guest, size);
#endif

      memset(page->validity, 1, size / search->params.value_size);

      cl_search_step_page(page, search->params, function, NULL);

      /* If there were no matches, reuse the allocated page */
      if (page->matches > 0)
      {
        if (!prev_page)
          page_region->first_page = page;
        else
          prev_page->next = page;

        prev_page = page;
        page_region->matches += page->matches;
        search->total_matches += page->matches;
        page_region->page_count++;
        search->total_page_count++;

        page = NULL;
      }

      processed += size;

#if CL_EXTERNAL_MEMORY
      bucket_processed += size;
      if (bucket_processed >= bucket_size)
      {
        bucket_offset += bucket_size;
        if (bucket_offset < page_region->region->size)
        {
          bucket_size = page_region->region->size - bucket_offset;
          if (bucket_size > CL_SEARCH_BUCKET_SIZE)
            bucket_size = CL_SEARCH_BUCKET_SIZE;

          cl_read_memory_buffer(bucket, page_region->region, bucket_offset, bucket_size);
        }
        bucket_processed = 0;
      }
#endif
    }

    if (page && page->matches == 0)
      cl_search_free_page(search, page);
    search->memory_scanned += page_region->region->size;
  }

  search->steps = 1;

#if CL_EXTERNAL_MEMORY
  cl_munmap(bucket, CL_SEARCH_BUCKET_SIZE);
#endif
  cl_search_profile_memory(search);
  search->time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;

  cl_search_step_print(search);

  return cl_search_profile_memory(search);
}

cl_error cl_search_step(cl_search_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else if (search->steps == 0)
    return cl_search_step_first(search);
  else
  {
    cl_search_compare_func_t function;
    cl_addr_t total_matches = 0;
    clock_t start = clock();
    void *prev_buffer = malloc(CL_SEARCH_CHUNK_SIZE);
    unsigned i;

    search->time_taken = clock();
    search->memory_scanned = 0;

    for (i = 0; i < search->page_region_count; i++)
    {
      cl_search_page_region_t *page_region = &search->page_regions[i];
      cl_search_page_t *page = page_region->first_page;
      cl_search_page_t *prev_page = NULL;
      cl_search_page_t *next_page = NULL;
      cl_addr_t page_region_matches = 0;

      function = cl_search_comparison_function(search->params, page_region->region->endianness);
      if (!function)
        continue;

      while (page)
      {
        cl_error error;
        
        memcpy(prev_buffer, page->chunk, page->size);
        cl_read_memory_buffer(page->chunk, page->region,
          page->start - page->region->base_guest, page->size);
        error = cl_search_step_page(page, search->params, function, prev_buffer);
        search->memory_scanned += page->size;

        if (error != CL_OK)
          goto error;
        else if (page->matches == 0)
        {
          /* Remove this page from the linked list */
          if (prev_page)
            prev_page->next = page->next;
          else
            page_region->first_page = page->next;

          next_page = page->next;
          cl_search_free_page(search, page);
          page_region->page_count--;
          search->total_page_count--;
          page = next_page;
        }
        else
        {
          /* Keep this page in the linked list */
          if (!prev_page)
            page_region->first_page = page;
          prev_page = page;
          page_region_matches += page->matches;
          page = page->next;
        }
      }
      if (prev_page)
        prev_page->next = NULL;
      else
        page_region->first_page = NULL;
      page_region->matches = page_region_matches;
      total_matches += page_region_matches;
    }
    search->total_matches = total_matches;
    search->steps++;
    cl_search_profile_memory(search);
    search->time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    cl_search_step_print(search);
    free(prev_buffer);

    return CL_OK;

    error:
    free(prev_buffer);
    return CL_ERR_CLIENT_RUNTIME;
  }
}

cl_error cl_search_remove(cl_search_t *search, cl_addr_t address)
{
  cl_search_page_region_t *page_region;
  cl_search_page_t *page;
  unsigned i;

  if (!search)
    return CL_ERR_PARAMETER_NULL;

  for (i = 0; i < search->page_region_count; i++)
  {
    page_region = &search->page_regions[i];

    /* Is it in this region? */
    if (address < page_region->region->base_guest ||
        address >= page_region->region->base_guest + page_region->region->size)
      continue;

    page = page_region->first_page;

    while (page)
    {
      /* Is it in this page? */
      if (address >= page->start && address < page->start + page->size)
      {
        cl_addr_t offset = address - page->start;
        unsigned char *target = &page->validity[offset / search->params.value_size];

        if (*target)
        {
          page->validity[offset / search->params.value_size] = 0;
          page->matches--;
          if (page->matches == 0)
            cl_search_free_page(search, page);

          return CL_OK;
        }
      }
      page = page->next;
    }
  }

  return CL_ERR_PARAMETER_INVALID;
}

cl_error cl_search_reset(cl_search_t *search)
{
  if (!search)
    return CL_ERR_PARAMETER_NULL;
  else
  {
    cl_search_parameters_t params = search->params;
    cl_error error = cl_search_free(search);

    if (error)
      return error;
    search->params = params;

    return CL_OK;
  }
}

cl_error cl_search_backup_value(void *dst, const cl_search_t *search,
  cl_addr_t address)
{
  cl_search_page_region_t *page_region;
  cl_search_page_t *page;
  unsigned i;

#if CL_SAFETY
  if (!search || !dst)
    return CL_ERR_PARAMETER_NULL;
#endif

  for (i = 0; i < search->page_region_count; i++)
  {
    page_region = &search->page_regions[i];

    /* Is it in this region? */
    if (address < page_region->region->base_guest ||
        address >= page_region->region->base_guest + page_region->region->size)
      continue;

    page = page_region->first_page;

    while (page)
    {
      /* Is it in this page? */
      if (address >= page->start && address < page->start + page->size)
      {
        return cl_read_value(dst, page->chunk, address - page->start,
                             search->params.value_type,
                             page->region->endianness);
      }
      page = page->next;
    }
  }

  return CL_ERR_PARAMETER_INVALID;
}
