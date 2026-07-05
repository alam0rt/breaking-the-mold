/* =============================================================================
 * gamestate_vtable.c  --  PC-port GameState vtable (D_80012100)
 * =============================================================================
 * Strong backing for the rodata vtable at 0x80012100 (asm/data/
 * vehicle_b.rodata.s), which _autoglobals would otherwise weak-zero. This is
 * the vtable ConstructStaticGameState installs at GameState+0x18 (src/gstate.c
 * InitEntityWithTableAndSprite, where it is aliased DEAD_ENTITY_VTABLE -- a
 * misleading clean-room name). The frame loop's post-render dispatch
 * ((**(gs->postRenderCallbackContext + 0x1C))(gs + argOff)) lands in slot 7 =
 * EntityRemoval, the per-frame texture/CLUT upload pass.
 *
 * Layout (four 8-byte {arg, fn} pairs, same shape as the entity vtables in
 * gfx.rodata.c):
 *   +0x0C DestroyEntityAndFreeResources   (destroy)
 *   +0x14 MainNoopCallback_80082844       (tick: no-op)
 *   +0x1C EntityRemoval                   (post-render upload pass)
 * ========================================================================== */

extern char DestroyEntityAndFreeResources[];
extern char MainNoopCallback_80082844[];
extern char EntityRemoval[];

void *D_80012100_vtable[8] asm("D_80012100") = {
    0,
    0,
    0,
    (void *)DestroyEntityAndFreeResources,
    0,
    (void *)MainNoopCallback_80082844,
    0,
    (void *)EntityRemoval
};
