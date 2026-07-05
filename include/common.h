/* =============================================================================
 * PSX Common Header
 * =============================================================================
 * Common types and definitions for PSX development.
 * Based on the original PSY-Q SDK headers.
 * =============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

/* -----------------------------------------------------------------------------
 * Standard integer types (PSX convention)
 * -------------------------------------------------------------------------- */

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

/* Boolean type */
typedef s32 bool;
#define TRUE 1
#define FALSE 0

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* -----------------------------------------------------------------------------
 * PSX-specific types
 * -------------------------------------------------------------------------- */

/* Fixed-point types (common in PSX games) */
typedef s16 fixed16;    /* 1.15 fixed point */
typedef s32 fixed32;    /* 16.16 fixed point */

/* BSD-style types (used by PSY-Q headers) */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

/* -----------------------------------------------------------------------------
 * PSY-Q SDK headers (if available)
 * -------------------------------------------------------------------------- */
#if __has_include("LIBAPI.H")
#include "LIBAPI.H"
#endif
#if __has_include("LIBGTE.H")
#include "LIBGTE.H"
#endif
#if __has_include("LIBGPU.H")
#include "LIBGPU.H"
#endif
#if __has_include("LIBCD.H")
#include "LIBCD.H"
#endif
#if __has_include("LIBSPU.H")
#include "LIBSPU.H"
#endif
#if __has_include("LIBETC.H")
#include "LIBETC.H"
#endif
#if __has_include("LIBPAD.H")
#include "LIBPAD.H"
#endif

/* -----------------------------------------------------------------------------
 * Compiler attributes
 * -------------------------------------------------------------------------- */

/* Force function to be inlined (GCC 2.6 compatible) */
#define INLINE static inline

/* Prevent function from being inlined */
#define NOINLINE __attribute__((noinline))

/* Align data to boundary */
#define ALIGNED(x) __attribute__((aligned(x)))

/* Mark as used (prevent dead code elimination) */
#define USED __attribute__((used))

/* -----------------------------------------------------------------------------
 * Register-pin / scheduling-fence shims (byte-neutral on MIPS)
 * -------------------------------------------------------------------------- *
 * A handful of matched functions pin a value to a specific MIPS register to
 * reproduce the original scheduler, e.g.
 *     register s16 m1 asm("$17");
 * Those pins are MIPS-only and meaningless on x86. They are written through the
 * PSX_REG() wrapper so the PC port (TARGET_PC) drops the `asm("$N")` clause
 * while the MIPS build expands to the identical tokens -> byte-identical output.
 * PSX_FENCE() is the empty-`__asm__` scheduling barrier equivalent, and
 * PSX_CLOBBER("$N") wraps an empty-asm with a register-clobber list (used to
 * force cc1 to re-materialize a value per branch arm). All three vanish on x86.
 *     register s16 m1 PSX_REG("$17");   // -> `... asm("$17")` on MIPS, `...` on PC
 *     PSX_CLOBBER("$5");                // -> `__asm__ __volatile__("" ::: "$5")` on MIPS
 */
#ifdef TARGET_PC
#define PSX_REG(n)
#define PSX_FENCE() ((void)0)
#define PSX_CLOBBER(r) ((void)0)
#else
#define PSX_REG(n) asm(n)
#define PSX_FENCE() __asm__ __volatile__("")
#define PSX_CLOBBER(r) __asm__ __volatile__("" ::: r)
#endif

/* -----------------------------------------------------------------------------
 * Include ASM macro
 * -------------------------------------------------------------------------- */
/* 
 * splat generates include/include_asm.h with the proper INCLUDE_ASM macro.
 * If generate_asm_macros_files is enabled in your splat yaml, use that instead.
 * This is a fallback definition.
 */

#ifndef INCLUDE_ASM
/* Include splat-generated macro if available */
#if __has_include("include_asm.h")
#include "include_asm.h"
#else
/* Fallback: Modern splat format for non-matching/WIP builds */
#define INCLUDE_ASM(DIR, FUNC) \
    __asm__( \
        ".section .text\n" \
        ".set noat\n" \
        ".set noreorder\n" \
        ".include \"" DIR "/" FUNC ".s\"\n" \
        ".set reorder\n" \
        ".set at\n" \
    )
#endif
#endif

/* -----------------------------------------------------------------------------
 * Utility macros
 * -------------------------------------------------------------------------- */

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) MIN(MAX(val, min), max)

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

/* Bit manipulation */
#define BIT(n) (1 << (n))
#define BITS(x, start, len) (((x) >> (start)) & ((1 << (len)) - 1))

/* Address manipulation */
#define LO16(addr) ((u16)((u32)(addr) & 0xFFFF))
#define HI16(addr) ((u16)(((u32)(addr) >> 16) & 0xFFFF))

#endif /* COMMON_H */
