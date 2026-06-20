#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

#if !defined(M2CTX) && !defined(PERMUTER)

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME) \
    void __maspsx_include_asm_hack_##NAME() { \
        __asm__( \
            ".text # maspsx-keep \n" \
            "    .align 2 # maspsx-keep\n" \
            "    .set noat # maspsx-keep\n" \
            "    .set noreorder # maspsx-keep\n" \
            "    .include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n" \
            "    .set reorder # maspsx-keep\n" \
            "    .set at # maspsx-keep\n" \
        ); \
    }
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME) \
    __asm__( \
        ".section .rodata\n" \
        "    .include \"" FOLDER "/" #NAME ".s\"\n" \
        ".section .text" \
    )
#endif

#if INCLUDE_ASM_USE_MACRO_INC
/* @match: injects splat's macro.inc into every TU so .include'd asm in INCLUDE_ASM() resolves. */
__asm__(".include \"include/macro.inc\"\n");
#else
/* @match: injects splat's labels.inc into every TU so .include'd asm in INCLUDE_ASM() resolves. */
__asm__(".include \"include/labels.inc\"\n");
#endif

#else

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)
#endif

#endif /* !defined(M2CTX) && !defined(PERMUTER) */

#endif /* INCLUDE_ASM_H */
