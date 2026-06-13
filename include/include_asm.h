#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

/*
 * maspsx-keep INCLUDE_ASM
 * --------------------------------------------------------------------------
 * This cc1 generation (GCC 2.6/2.7) defers every compiled function body to the
 * end of the translation unit, while a bare top-level `__asm__` block streams
 * immediately. A classic `INCLUDE_ASM` (top-level asm) therefore floats ABOVE
 * the compiled C functions, reordering .text and breaking the byte-match — so
 * historically only a contiguous *tail* of a module could be decompiled
 * (see docs/compiler-quirks.md Quirk 1).
 *
 * The fix (from Vatuu/silent-hill-decomp, supported by tools/maspsx): wrap the
 * `.include` in a function named `__maspsx_include_asm_hack_<NAME>` and tag
 * every asm line `# maspsx-keep`. cc1 then keeps it in the deferred-function
 * stream IN SOURCE ORDER alongside the real C functions; maspsx strips the
 * wrapper back to a plain in-place `.include`. Matched C and INCLUDE_ASM can
 * then interleave freely within one file.
 */

#if !defined(M2CTX) && !defined(PERMUTER)

/* Make glabel/endlabel/jlabel etc. available to the included per-function asm.
 * macro.inc is include-guarded, so pulling it here is harmless for pure-C
 * translation units. */
__asm__(".include \"include/macro.inc\"\n"
        "\t.set reorder\n"
        "\t.set at\n");

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)                                     \
    void __maspsx_include_asm_hack_##NAME(void)                       \
    {                                                                 \
        __asm__(".text # maspsx-keep\n"                               \
                "\t.align\t2 # maspsx-keep\n"                         \
                "\t.set noreorder # maspsx-keep\n"                    \
                "\t.set noat # maspsx-keep\n"                         \
                ".include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n" \
                "\t.set reorder # maspsx-keep\n"                      \
                "\t.set at # maspsx-keep\n");                         \
    }
#endif

#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)                                  \
    void __maspsx_include_rodata_hack_##NAME(void)                    \
    {                                                                 \
        __asm__(".section .rodata # maspsx-keep\n"                    \
                ".include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n" \
                ".section .text # maspsx-keep\n");                    \
    }
#endif

#else /* M2CTX / PERMUTER: strip asm includes so the C context parses. */

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)
#endif

#endif /* !defined(M2CTX) && !defined(PERMUTER) */

#endif /* INCLUDE_ASM_H */
