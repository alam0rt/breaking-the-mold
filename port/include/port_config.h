/* =============================================================================
 * port_config.h  --  PC-port build configuration (TARGET_PC)
 * =============================================================================
 * Included automatically into every translation unit of the PC build via the
 * CMake `-include port_config.h` flag, BEFORE any project header. It:
 *   1. asserts the 32-bit ABI the game structs/vtables/assets assume,
 *   2. neutralises the decomp's MIPS-specific matching artifacts
 *      (register pins, scheduling barriers) so the shared src/*.c parse on x86,
 *   3. turns INCLUDE_ASM into a no-op (the symbols are resolved from the
 *      generated weak stub TU, or from real functional-C bodies once written).
 *
 * The MIPS byte-match build never defines TARGET_PC and never sees this file,
 * so `make check` is completely unaffected.  See port/README.md.
 * ========================================================================== */
#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

#ifndef TARGET_PC
#define TARGET_PC 1
#endif

/* --- 1. ABI guard --------------------------------------------------------- */
/* The engine overlays pointers at fixed byte offsets and strides 8-byte
 * callback slots (see include/Game/entity.h, fsm_dispatch.h). A 64-bit build
 * silently corrupts every one of those layouts, so we hard-fail the build. */
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ != 4)
#error "The PC port must be built 32-bit (-m32): game structs assume 4-byte pointers."
#endif

/* --- 2. neutralise decomp matching artifacts ------------------------------ */
/* fsm_dispatch.h already offers a behaviour-only mode; select it. This kills
 * the FSM_REG/FSM_KEEP_LIVE/FSM_RELAY register pins used across the engine. */
#ifndef FSM_PORTABLE
#define FSM_PORTABLE 1
#endif

/* The bare `register T x asm("$N")` pins scattered in
 * playst.c/enemies.c/vram.c/ending.c/menu.c are swept (tools/portize.py) to the
 * PSX_REG("$N") wrapper. PSX_REG / PSX_FENCE themselves are defined in
 * include/common.h gated on TARGET_PC (which we set above), so they vanish on
 * x86 while staying byte-identical on MIPS. Nothing to define here. */

/* --- 3. INCLUDE_ASM becomes a no-op on PC --------------------------------- */
/* Defined here (before common.h / include_asm.h) so their #ifndef guards skip
 * the asm-emitting versions. Each shelved function's symbol is satisfied by a
 * weak stub in port/decomp/_autostubs.c, overridden as real bodies land. */
#define INCLUDE_ASM(FOLDER, NAME)
#define INCLUDE_RODATA(FOLDER, NAME)

/* --- misc ----------------------------------------------------------------- */
/* Trap for any HAL/game function that is still an unimplemented stub. */
void port_stub(const char *fn);

#endif /* PORT_CONFIG_H */
