/* =============================================================================
 * MenuActivateButtonWithReset.c  --  PC-port menu button activation (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/menu/MenuActivateButtonWithReset.s
 * (0x800757AC, 47 lines; reference = export/SLES_010.90.c 100857). Activates a
 * menu button's child cursor entity (at button+0x100): marks the button active
 * (+0x104 = 1), installs the button-input event callback + highlighted sprite,
 * queues the idle-anim next-state, and hides the button's own sprite-context
 * primitive (spriteContext+0xA = 0).
 *
 * Offsets from the disassembly:
 *   child = *(button+0x100); button+0x104 = 1;
 *   child+0x08 = 0xFFFF0000 (event marker), child+0x0C = MenuButtonCallback;
 *   SetEntitySpriteId(child, 0x63848E59, 1);
 *   child+0x98 = 0xFFFF0000 (next-state marker), child+0x9C = MenuSetEntityIdle2;
 *   *(button+0x34) + 0x0A = 0.
 * ========================================================================== */
#include "common.h"

extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);
extern void MenuButtonCallback(void);    /* referenced by address only */
extern void MenuSetEntityIdle2(void);

void MenuActivateButtonWithReset(void *entity) {
    u8 *button = (u8 *)entity;
    u8 *child = *(u8 **)(button + 0x100);

    button[0x104] = 1;
    *(u32 *)(child + 0x08) = 0xFFFF0000;
    *(void **)(child + 0x0C) = (void *)MenuButtonCallback;
    SetEntitySpriteId(child, 0x63848E59, 1);
    *(u32 *)(child + 0x98) = 0xFFFF0000;
    *(void **)(child + 0x9C) = (void *)MenuSetEntityIdle2;
    ((u8 *)(*(void **)(button + 0x34)))[0x0A] = 0;
}
