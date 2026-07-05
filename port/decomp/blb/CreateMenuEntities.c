/* =============================================================================
 * CreateMenuEntities.c  --  PC-port HUD/menu entity factory (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/CreateMenuEntities.s
 * (0x800281A4, 0x21D4 bytes; reference = export/SLES_010.90.c 24362-25178).
 *
 * Populates a freshly heap-allocated 0xB0-byte HUD container (param_1) with the
 * ~30 child entities that make up the in-game score/lives/powerup readout:
 *   - the container itself gets the menu vtable D_8001050C, its input-state
 *     pointer at +0x1C, and two {0xFFFF0000, fn} FSM slots (tick/event) wired to
 *     HUD_UpdateVisibilityWrapper / EntityCollision_HUDItemActivate;
 *   - a run of sprite/"digit"/"HUD-item" child entities, each allocated from the
 *     BLB heap, built with InitEntitySprite, given the shared platform-ride idle
 *     skeleton (see mk_sprite_entity), then specialised;
 *   - three InitEntityWithSprite children (slots 0x25..0x27) for the timer/icon
 *     art from menu spawn-data D_8009B174/D_8009B180/D_8009B18C;
 *   - a trailing zero-fill of the container's per-child flag bytes 0xA2..0xAC.
 * Returns param_1 (the caller stores it as GameState.hud_entity_ptr).
 *
 * Clean-room RE: every symbol name is an inferred working label. Struct layout
 * is expressed as byte-offset pointer arithmetic (Entity stride 0x80,
 * BasicEntity/spriteContext stride 0x10) transcribed from the disassembly; no
 * struct types are assumed. The 5-argument form of InitEntityWithSprite and the
 * D_8009B174/0x271A/0xA0/0x78 argument values were recovered from the asm
 * (the Ghidra export mis-renders it as a 2-arg call).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

/* ---- heap / entity construction ------------------------------------------- */
extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void  InitMenuEntityWithVtable(void *e, s32 prio);
extern void *InitEntitySprite(void *e, u32 spriteId, s16 z, s16 x, s16 y, u8 mode);
extern void  InitEntityWithSprite(void *e, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY);
extern void  EntitySetState(void *e, u32 marker, void *fn);
extern void  UpdateDigitDisplay(void *e);
extern void  UpdateHUDItemVisibility(void *e);
extern void *InitEntity_8c510186(void *e);
extern void *InitTimerDisplayEntity(void *e);
extern s32   GetTPage(s32 tp, s32 abr, s32 x, s32 y);

/* ---- callbacks referenced only by address --------------------------------- */
extern void StubReturnZero(void);
extern void EntityTick_PlatformRideIdle(void);
extern void EntityTick_DigitDisplayUpdate(void);
extern void EntityTick_HUDItemUpdate(void);
extern void GetWorldPositionX(void);
extern void GetWorldPositionY(void);
/* Port forwarder for the HUD-visibility tick trampoline: the MIPS
 * HUD_UpdateVisibilityWrapper relies on a $a0 register passthrough that does
 * not survive x86 cdecl, so the port installs this arg-taking shim instead. */
extern void port_HUD_UpdateVisibilityWrapper(void *entity);
extern void EntityCollision_HUDItemActivate(void);

/* ---- FSM default-state slot pair (real C globals in src/blb.c) ------------- *
 * D_800A5988 = 0xFFFF0000 marker, D_800A598C = PlatformRideComplete fn.        */
extern u32   D_800A5988;
extern void *D_800A598C;

/* ---- entity collision vtables (real C arrays in port/decomp/data/gfx.rodata.c) */
extern void *D_8001050C[];
extern void *D_8001058C[];
extern void *D_800105AC[];
extern void *D_800105CC[];
extern void *D_800105EC[];

/* ---- menu spawn-data pointers (weak-backed by gen_port_stubs.py) ----------- */
extern u8 D_8009B174[] asm("D_8009B174");
extern u8 D_8009B180[] asm("D_8009B180");
extern u8 D_8009B18C[] asm("D_8009B18C");

/* g_pPlayerState / the PlayerState struct come from include/globals.h ->
 * include/Game/player_state.h. Its storage is defined and seeded once at boot
 * (port/decomp/boot/game_boot.c), so nothing needs to be re-declared here. */

/* -----------------------------------------------------------------------------
 * mk_sprite_entity -- the shared skeleton every InitEntitySprite child begins
 * with, up to and including the EntitySetState hand-off. Allocates the entity,
 * builds its sprite, installs the type-0 collision vtable (D_800105CC), the
 * platform-ride idle tick callback, the stub event callback, and the
 * world-position move callbacks, then drives it into the default FSM state.
 *
 * The `*(u8*)(e+0x91)=0` write that follows EntitySetState in every block is
 * kept here (all post-EntitySetState writes target distinct, non-aliasing
 * offsets, so ordering is behaviourally irrelevant). Callers then overwrite the
 * few fields that differ per child (vtable, tick callback, digit/HUD state).
 * -------------------------------------------------------------------------- */
static u8 *mk_sprite_entity(s32 allocSize, u32 hash, s16 x, s16 y,
                            u16 tickcfg, u16 eventMarker)
{
    u8 *e = AllocateFromHeap((u8 *)g_pBlbHeapBase, allocSize, 1, 0);

    InitEntitySprite(e, hash, 10000, x, y, 1);
    *(void **)(e + 0x18) = (void *)D_800105CC;            /* collisionVtable */
    *(s32  *)(e + 0x08) = -0x10000;                       /* eventMarker */
    *(void **)(e + 0x0C) = (void *)StubReturnZero;        /* eventCallback */
    *(s32  *)(e + 0x00) = -0x10000;                       /* tickMarker */
    *(void **)(e + 0x04) = (void *)EntityTick_PlatformRideIdle;
    *(s32  *)(e + 0x24) = -0x10000;                       /* moveMarkerX */
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32  *)(e + 0x2C) = -0x10000;                       /* moveMarkerY */
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u8   *)(e + 0x14) = 0;                              /* active */
    *(u8   *)(e + 0x8E) = 0;                              /* &[1].eventCallback+2 */
    *(u16  *)(e + 0x8C) = 0;                              /* &[1].eventCallback */
    *(u16  *)(e + 0x86) = tickcfg;                        /* &[1].tickCallback+2 */
    *(u16  *)(e + 0x88) = eventMarker;                    /* &[1].eventMarker */
    *(u8   *)(e + 0x90) = 1;                              /* &[1].allocSize */
    EntitySetState(e, D_800A5988, D_800A598C);
    *(u8   *)(e + 0x91) = 0;                              /* &[1].allocSize+1 */
    return e;
}

/* -----------------------------------------------------------------------------
 * apply_sprite_tpage -- compute and cache the sprite's TPage id from its
 * BasicEntity/spriteContext (stride 0x10): tp = GetTPage(sc[3].screenY, 1,
 * sc[1].screenX & ~0x3F, sc[1].screenY & ~0xFF); sc[2].widthOrU = tp.
 * -------------------------------------------------------------------------- */
static void apply_sprite_tpage(u8 *e)
{
    u8 *sc = *(u8 **)(e + 0x34);
    u16 tp = (u16)GetTPage(*(u8 *)(sc + 0x32), 1,
                           *(s16 *)(sc + 0x10) & ~0x3F,
                           *(s16 *)(sc + 0x12) & ~0xFF);
    *(u16 *)(sc + 0x24) = tp;
}

/* -----------------------------------------------------------------------------
 * finish_digit -- the specialisation shared by every "digit" child: swap in the
 * digit collision vtable (D_800105AC), stamp its render marker and per-digit
 * config bytes, splat the (u16)count value across the four active/pad bytes
 * (high byte is always 0 for the u8 counters), then install the digit tick
 * callback and prime the display via UpdateDigitDisplay.
 * -------------------------------------------------------------------------- */
static void finish_digit(u8 *e, u16 renderMarker, u8 vtHi, u16 vtLo, u8 count)
{
    u16 v = (u16)count;

    *(void **)(e + 0x18) = (void *)D_800105AC;   /* collisionVtable */
    *(u16 *)(e + 0x9C) = renderMarker;           /* &[1].renderMarker */
    *(u8  *)(e + 0x9B) = vtHi;                    /* &[1].collisionVtable+3 */
    *(u8  *)(e + 0x94) = (u8)v;                   /* &[1].active */
    *(u8  *)(e + 0x95) = (u8)(v >> 8);            /* &[1].pad15 */
    *(u8  *)(e + 0x96) = (u8)v;                   /* &[1].pad16 */
    *(u8  *)(e + 0x97) = (u8)(v >> 8);            /* &[1].pad17 */
    *(u16 *)(e + 0x98) = vtLo;                    /* &[1].collisionVtable */
    *(s32 *)(e + 0x00) = -0x10000;               /* tickMarker */
    *(void **)(e + 0x04) = (void *)EntityTick_DigitDisplayUpdate;
    UpdateDigitDisplay(e);
    *(u8  *)(e + 0x9A) = 10;                      /* &[1].collisionVtable+2 */
}

/* -----------------------------------------------------------------------------
 * finish_huditem -- the specialisation shared by the loop's two HUD-item
 * children: swap in the HUD-item collision vtable (D_8001058C), record the
 * collectible count and slot index, install the HUD-item tick callback, refresh
 * visibility, then hide the entity and mark its spriteContext flag byte.
 * -------------------------------------------------------------------------- */
static void finish_huditem(u8 *e, u8 count, u8 idx)
{
    u8 *sc;

    *(void **)(e + 0x18) = (void *)D_8001058C;   /* collisionVtable */
    *(u8 *)(e + 0x94) = count;                    /* &[1].active */
    *(u8 *)(e + 0x95) = count;                    /* &[1].pad15 */
    *(u8 *)(e + 0x96) = idx;                      /* &[1].pad16 */
    *(s32 *)(e + 0x00) = -0x10000;               /* tickMarker */
    *(void **)(e + 0x04) = (void *)EntityTick_HUDItemUpdate;
    UpdateHUDItemVisibility(e);
    sc = *(u8 **)(e + 0x34);
    *(u8 *)(e + 0x97) = 10;                       /* &[1].pad17 */
    *(u8 *)(e + 0xF6) = 0;                        /* visibility */
    *(u8 *)(sc + 0x37) = 1;                       /* sc[3].heightOrV+1 */
}

void *CreateMenuEntities(u8 *param_1, void *param_2)
{
    u8 *e;
    s16 sVar10;
    u32 uVar11;
    u8  local_268;

    /* ---- container header (menu vtable, input ptr, tick/event FSM slots) ---- */
    InitMenuEntityWithVtable(param_1, 30000);
    *(void **)(param_1 + 0x18) = (void *)D_8001050C;                 /* [6]  vtable */
    *(void **)(param_1 + 0x1C) = param_2;                            /* [7]  input state */
    *(u32  *)(param_1 + 0x00) = 0xFFFF0000;                          /* [0]  tickMarker */
    *(void **)(param_1 + 0x04) = (void *)port_HUD_UpdateVisibilityWrapper;/* [1] tickCallback */
    *(u32  *)(param_1 + 0x08) = 0xFFFF0000;                          /* [2]  eventMarker */
    *(void **)(param_1 + 0x0C) = (void *)EntityCollision_HUDItemActivate; /* [3] */
    local_268 = 0;

    /* [8] orb HUD icon */
    e = mk_sprite_entity(0x114, 0xB8700CA1, 0x18, -0x20, 0x16, 0xFFE0);
    *(void **)(param_1 + 0x20) = e;
    apply_sprite_tpage(e);

    /* [9] orb count, ones digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x25, -0x20, 0x20, 0xFFE0);
    finish_digit(e, 0x25, 0x00, 10, g_pPlayerState->orb_count);
    *(void **)(param_1 + 0x24) = e;

    /* [0xa] orb count, tens digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x31, -0x20, 0x20, 0xFFE0);
    finish_digit(e, 0x31, 0x0C, 0, g_pPlayerState->orb_count);
    *(void **)(param_1 + 0x28) = e;

    /* [0xb] label sprite */
    e = mk_sprite_entity(0x114, 0xA9240484, 0x118, -0x20, 0x0F, 0xFFE0);
    *(void **)(param_1 + 0x2C) = e;

    /* [0xc] lives count, ones digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0xFF, -0x20, 0x20, 0xFFE0);
    finish_digit(e, 0xFF, 0x00, 10, g_pPlayerState->lives);
    *(void **)(param_1 + 0x30) = e;

    /* [0xd] lives count, tens digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x10B, -0x20, 0x20, 0xFFE0);
    finish_digit(e, 0x10B, 0x00, 0, g_pPlayerState->lives);
    *(void **)(param_1 + 0x34) = e;

    /* fix up the [0xb] label sprite: hide it and clear a spriteContext flag */
    {
        u8 *b = *(u8 **)(param_1 + 0x2C);
        *(u8 *)(b + 0xF6) = 0;
        *(u8 *)(*(u8 **)(b + 0x34) + 0x37) = 0;
    }

    /* [0xe] swirly-Q HUD icon */
    e = mk_sprite_entity(0x114, 0xE8628689, 0x98, -0x20, 0x16, 0xFFE0);
    *(void **)(param_1 + 0x38) = e;
    apply_sprite_tpage(e);

    /* [0xf] swirly-Q count, ones digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x92, -0x20, 0x28, 0xFFE0);
    finish_digit(e, 0x92, 0x00, 10, g_pPlayerState->swirly_q_count);
    *(void **)(param_1 + 0x3C) = e;

    /* [0x10] swirly-Q count, tens digit */
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x9E, -0x20, 0x28, 0xFFE0);
    finish_digit(e, 0x9E, 0x06, 0, g_pPlayerState->swirly_q_count);
    *(void **)(param_1 + 0x40) = e;

    /* [0x11..0x13] + [0x14..0x16] three rows of {1970-icon, hamster} HUD items */
    while (1) {
        uVar11 = (u32)local_268;
        if (uVar11 > 2) {
            break;
        }

        e = mk_sprite_entity(0x118, 0x88A28194, 0x60, -0x20,
                             (u16)(local_268 * 0x10 + 0x16), 0xFFE0);
        finish_huditem(e, g_pPlayerState->icon_1970_count, local_268);
        *(void **)(param_1 + (uVar11 + 0x11) * 4) = e;

        e = mk_sprite_entity(0x118, 0x80E85EA0, 0xD0, -0x20,
                             (u16)(local_268 * 0x14 + 0x1C), 0xFFE0);
        finish_huditem(e, g_pPlayerState->hamster_count, local_268);
        *(void **)(param_1 + (uVar11 + 0x14) * 4) = e;

        local_268 = local_268 + 1;
    }

    /* [0x17] timer/frame label sprite (scaled 0x8000) */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x114, 0x9158A0F6, 0x18, sVar10, 200, (u16)sVar10);
    *(void **)(param_1 + 0x5C) = e;
    *(s32 *)(e + 0x50) = 0x8000;                 /* scaleRender */
    *(s32 *)(e + 0x54) = 0x8000;                 /* scaleRender2 */

    /* [0x18] phoenix-hands count, ones digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x25, sVar10, 0xD2, (u16)sVar10);
    finish_digit(e, 0x25, 0x00, 10, g_pPlayerState->phoenix_hands);
    *(void **)(param_1 + 0x60) = e;

    /* [0x19] phoenix-hands count, tens digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x31, sVar10, 0xD2, (u16)sVar10);
    finish_digit(e, 0x31, 0x0C, 0, g_pPlayerState->phoenix_hands);
    *(void **)(param_1 + 0x64) = e;

    /* [0x1b] specialised child (built entirely by InitEntity_8c510186) */
    e = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x114, 1, 0);
    InitEntity_8c510186(e);
    *(void **)(param_1 + 0x6C) = e;

    /* [0x1a] label sprite with hand-set spriteContext tile fields */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x114, 0xA9240484, 0x0E, sVar10, 0x88, (u16)sVar10);
    *(void **)(param_1 + 0x68) = e;
    *(u8 *)(e + 0xF6) = 0;                        /* visibility */
    {
        u8 *sc = *(u8 **)(e + 0x34);
        *(u8  *)(sc + 0x34) = 0x20;
        *(u8  *)(sc + 0x35) = 0x60;
        *(u8  *)(sc + 0x37) = 0;
        *(u8  *)(sc + 0x36) = 0x30;
        *(u16 *)(sc + 0x08) = 0x271A;
    }

    /* [0x1c] P-hart-heads count, ones digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x25, sVar10, 0xA2, (u16)sVar10);
    finish_digit(e, 0x25, 0x00, 10, g_pPlayerState->phart_heads);
    *(void **)(param_1 + 0x70) = e;

    /* [0x1d] P-hart-heads count, tens digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x31, sVar10, 0xA2, (u16)sVar10);
    finish_digit(e, 0x31, 0x0C, 0, g_pPlayerState->phart_heads);
    *(void **)(param_1 + 0x74) = e;

    /* [0x1e] timer display (built entirely by InitTimerDisplayEntity) */
    e = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x114, 1, 0);
    e = (u8 *)InitTimerDisplayEntity(e);
    *(void **)(param_1 + 0x78) = e;

    /* [0x1f] universe-enemas count, ones digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0xFF, sVar10, 0xD2, (u16)sVar10);
    finish_digit(e, 0xFF, 0x00, 10, g_pPlayerState->universe_enemas);
    *(void **)(param_1 + 0x7C) = e;

    /* [0x20] universe-enemas count, tens digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x10B, sVar10, 0xD2, (u16)sVar10);
    finish_digit(e, 0x10B, 0x00, 0, g_pPlayerState->universe_enemas);
    *(void **)(param_1 + 0x80) = e;

    /* [0x21] label sprite */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x114, 0x902C0002, 0x118, sVar10, 0x88, (u16)sVar10);
    *(void **)(param_1 + 0x84) = e;

    /* [0x22] super-willies count, ones digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0xFF, sVar10, 0xA2, (u16)sVar10);
    finish_digit(e, 0xFF, 0x00, 10, g_pPlayerState->super_willies);
    *(void **)(param_1 + 0x88) = e;

    /* [0x23] super-willies count, tens digit */
    sVar10 = (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20);
    e = mk_sprite_entity(0x120, 0x00E2F188, 0x10B, sVar10, 0xA2, (u16)sVar10);
    finish_digit(e, 0x10B, 0x00, 0, g_pPlayerState->super_willies);
    *(void **)(param_1 + 0x8C) = e;

    /* [0x25] icon child from spawn-data D_8009B174 */
    e = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x104, 1, 0);
    InitEntityWithSprite(e, (s32)(intptr_t)D_8009B174, 0x271A, 0xA0, 0x78);
    *(void **)(e + 0x18) = (void *)D_800105EC;    /* collisionVtable */
    *(s32 *)(e + 0x24) = -0x10000;                /* moveMarkerX */
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32 *)(e + 0x2C) = -0x10000;                /* moveMarkerY */
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u8 *)(e + 0x14) = 0;                         /* active */
    apply_sprite_tpage(e);
    *(u8 *)(e + 0x101) = 1;                        /* &[2].tickMarker+1 */
    *(void **)(param_1 + 0x94) = e;

    /* [0x26] icon child from spawn-data D_8009B180 */
    e = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x104, 1, 0);
    InitEntityWithSprite(e, (s32)(intptr_t)D_8009B180, 0x271A, 0xA0, 0x78);
    *(void **)(e + 0x18) = (void *)D_800105EC;
    *(s32 *)(e + 0x24) = -0x10000;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32 *)(e + 0x2C) = -0x10000;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u8 *)(e + 0x14) = 0;
    apply_sprite_tpage(e);
    *(u8 *)(e + 0x101) = 1;
    *(void **)(param_1 + 0x98) = e;

    /* [0x27] icon child from spawn-data D_8009B18C */
    e = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x104, 1, 0);
    InitEntityWithSprite(e, (s32)(intptr_t)D_8009B18C, 0x271A, 0xA0, 0x78);
    *(void **)(e + 0x18) = (void *)D_800105EC;
    *(s32 *)(e + 0x24) = -0x10000;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32 *)(e + 0x2C) = -0x10000;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u8 *)(e + 0x14) = 0;
    apply_sprite_tpage(e);
    *(u8 *)(e + 0x101) = 1;
    *(void **)(param_1 + 0x9C) = e;

    /* ---- clear the container's per-child flag bytes 0xA2..0xAC ---- */
    *(u8 *)(param_1 + 0xA2) = 0;
    *(u8 *)(param_1 + 0xA3) = 0;
    *(u8 *)(param_1 + 0xA4) = 0;
    *(u8 *)(param_1 + 0xA5) = 0;
    *(u8 *)(param_1 + 0xA6) = 0;
    *(u8 *)(param_1 + 0xA7) = 0;
    *(u8 *)(param_1 + 0xA8) = 0;
    *(u8 *)(param_1 + 0xA9) = 0;
    *(u8 *)(param_1 + 0xAA) = 0;
    *(u8 *)(param_1 + 0xAB) = 0;
    *(u8 *)(param_1 + 0xAC) = 0;

    return param_1;
}
