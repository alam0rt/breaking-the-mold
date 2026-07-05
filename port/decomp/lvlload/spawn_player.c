/* =============================================================================
 * spawn_player.c  --  PC-port player + initial-entity spawner (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/lvlload/SpawnPlayerAndEntities.s
 * (0x8007DF38, 448 lines; reference = export/SLES_010.90.c 109650) -- the final
 * step of InitGameState. Sets up per-level spawn state, then dispatches on the
 * level flags to create the correct player-avatar entity (normal / GLIDE / SOAR
 * / RUNN / results / MENU / FINN), a camera entity, the HUD, and any menu
 * sprite, registering each on the engine lists.
 *
 * Level-flag -> avatar dispatch (GetLevelFlags):
 *   0x400 -> FINN swamp-boat (0x114)          [outer else]
 *   0x200 -> MENU entity via InitMenuEntity (0x140)
 *   0x2000 -> results/ending screen (0x158)
 *   0x100 -> RUNN auto-scroller (0x110)
 *   0x010 -> SOAR flyer (0x128) + camera
 *   0x004 -> GLIDE (0x11C)
 *   else  -> normal platformer (0x1B4) + camera when not frozen
 * Every avatar stores the InputState ptr into player+0x100 (Entity[2].tickMarker)
 * and is recorded at GameState player_entity_ptr(+0x30). Common tail: camera set,
 * render-list add, sprite-asset load, optional menu sprite, HUD entity, and a
 * one-shot HUD event dispatch (0x1013) when the level has a real asset index.
 *
 * GameState offsets (from export/datatypes.txt) and the branch structure are
 * transcribed faithfully. Entity-creation callees (Create*PlayerEntity,
 * InitMenuEntity, CreateMenuEntities, ...) are still-asm weak stubs until
 * converted -- the MENU boot path reaches InitMenuEntity first.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

/* The avatar-creation calls take the player-1 InputState pointer loaded from
 * D_800A5964 (the decompiler mislabels this g_pPlayer1Input; the globals.h
 * symbol of that name is a different address, 0x800A5764). Alias the real
 * D_800A5964 slot here -- same one game_boot.c stores as g_pInputStateA. */
extern void *s_p1InputSlot asm("D_800A5964");   /* value = InputState* (p1)   */

extern u16   GetLevelFlags(void *ctx);
extern u16   GetTileHeaderField1A(void *ctx);
extern u8    GetCurrentLevelAssetIndex(void *ctx);
extern u8    GetTileAttributeAtPosition(s32 gs, s32 x, s32 y);
extern u8    GetSlopeHeightAtSubpixel(void *gs, u32 attr, s32 x);
extern void  ClearEntityPoolArray(s32 gs);
extern u8   *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *CreatePlayerEntity(void *e, void *input, s16 x, s16 y, s32 facing);
extern void *CreateGlidePlayerEntity(void *e, void *input, s16 x, s16 y);
extern void *CreateSoarPlayerEntity(void *e, void *input, s16 x, s16 y);
extern void *CreateRunnPlayerEntity(void *e, void *input, s16 x, s16 y);
extern void *CreateResultsScreenEntity(void *e, void *input, u8 bossType);
extern void *CreateFinnPlayerEntity(void *e, void *input, s16 x, s16 y);
extern void *InitMenuEntity(void *e, s32 input, s32 pwLevels, u8 pwCount);
extern void *CreateCameraEntity(void *e, s16 x, s16 y, s32 speed);
extern void  SetCameraPositionDirect(void *gs, s16 x, s16 y);
extern void  AddEntityToSortedRenderList(void *gs, void *entity);
extern void  LoadLevelSpriteAssets(s32 gs);
extern s32   InitMenuSpriteEntity(void *e);
extern void  AddToZOrderList(void *gs, s32 entity);
extern void  AddToXPositionList(void *gs, s32 obj);
extern void *CreateMenuEntities(void *e, void *input);

void SpawnPlayerAndEntities(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *ctx = gs + 0x84;
    u8 *player;
    s16 camY;
    u16 flags;
    u8  attr;

    flags = GetLevelFlags(ctx);
    gs[0x19C] = (u8)((flags >> 1) & 1);           /* boss_defeated       */
    gs[0x19D] = (u8)GetTileHeaderField1A(ctx);    /* boss_facing         */
    gs[0x170] = 1;                                 /* level_active        */
    if (GetCurrentLevelAssetIndex(ctx) == 0) {
        gs[0x170] = 0;
    }

    /* camera scroll speed by flags */
    {
        s32 speed = 0x8000;
        if ((GetLevelFlags(ctx) & 0x80) == 0) {
            speed = 0xC000;
            if ((GetLevelFlags(ctx) & 8) == 0) {
                speed = 0x10000;
            }
        }
        *(s32 *)(gs + 0x11C) = speed;
    }

    /* slope-snap the spawn Y if standing on a slope tile */
    attr = GetTileAttributeAtPosition((s32)(uintptr_t)gs, (s32)*(s16 *)(gs + 0x116),
              (s32)(s16)(u16)((*(s16 *)(gs + 0x118) + 1)));
    if (attr != 0x65 && attr != 0 && (u32)attr < 0x3C) {
        u8 h = GetSlopeHeightAtSubpixel(gs, (u32)attr, (s32)*(s16 *)(gs + 0x116));
        *(s16 *)(gs + 0x118) = (s16)(((*(s16 *)(gs + 0x118) + 1) & 0xFFF0) - (h - 0xF));
    }

    /* special modes (flags 0x20 / 0x04) get a 0x100-entry entity pool */
    {
        s32 pool = 0;
        if ((GetLevelFlags(ctx) & 0x20) != 0 || (GetLevelFlags(ctx) & 4) != 0) {
            pool = 1;
        }
        if (pool) {
            if (*(void **)(gs + 0x16C) == NULL) {
                *(void **)(gs + 0x16C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 4, 0x100, 0);
            }
            ClearEntityPoolArray((s32)(uintptr_t)gs);
        }
    }

    /* ---- avatar dispatch ---- */
    if ((GetLevelFlags(ctx) & 0x400) == 0) {
        if ((GetLevelFlags(ctx) & 0x200) == 0) {
            if ((GetLevelFlags(ctx) & 0x2000) == 0) {
                if ((GetLevelFlags(ctx) & 0x100) == 0) {
                    if ((GetLevelFlags(ctx) & 0x10) == 0) {
                        if ((GetLevelFlags(ctx) & 4) == 0) {
                            /* normal platformer player */
                            *(s16 *)(gs + 0x64) = 0x28;
                            *(s16 *)(gs + 0x66) = -0x30;
                            player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x1B4, 1, 0);
                            player = CreatePlayerEntity(player, s_p1InputSlot,
                                        *(s16 *)(gs + 0x116), *(s16 *)(gs + 0x118),
                                        ((GetLevelFlags(ctx) >> 12) ^ 1) & 1);
                            *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
                            *(void **)(gs + 0x30) = player;
                            if (gs[0x161] == 0 && (GetLevelFlags(ctx) & 0x1000) == 0) {
                                u8 *cam = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10C, 1, 0);
                                cam = CreateCameraEntity(cam, *(s16 *)(gs + 0x116),
                                                         *(s16 *)(gs + 0x118), 0);
                                AddEntityToSortedRenderList(gs, cam);
                                camY = 0x60;
                            } else {
                                camY = 0;
                            }
                        } else {
                            /* GLIDE player */
                            *(s16 *)(gs + 0x64) = 0;
                            *(s16 *)(gs + 0x66) = 0;
                            player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x11C, 1, 0);
                            CreateGlidePlayerEntity(player, s_p1InputSlot,
                                        *(s16 *)(gs + 0x116), *(s16 *)(gs + 0x118));
                            camY = 0;
                            *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
                            *(s16 *)(gs + 0x120) = 0;
                            *(s16 *)(gs + 0x122) = 0;
                            gs[0x12A] = 0;
                            *(void **)(gs + 0x30) = player;
                        }
                    } else {
                        /* SOAR flyer */
                        *(s16 *)(gs + 0x64) = 0x28;
                        *(s16 *)(gs + 0x66) = -0x60;
                        player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x128, 1, 0);
                        player = CreateSoarPlayerEntity(player, s_p1InputSlot,
                                    *(s16 *)(gs + 0x116), (s16)(*(s16 *)(gs + 0x118) - 0x80));
                        *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
                        *(void **)(gs + 0x30) = player;
                        {
                            u8 *cam = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10C, 1, 0);
                            cam = CreateCameraEntity(cam, *(s16 *)(gs + 0x116),
                                        (s16)(*(s16 *)(gs + 0x118) - 0x80), 0xA000);
                            AddEntityToSortedRenderList(gs, cam);
                        }
                        camY = 0x80;
                    }
                } else {
                    /* RUNN auto-scroller */
                    *(s16 *)(gs + 0x64) = 0x28;
                    *(s16 *)(gs + 0x66) = -0x30;
                    player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x110, 1, 0);
                    player = CreateRunnPlayerEntity(player, s_p1InputSlot,
                                *(s16 *)(gs + 0x116), *(s16 *)(gs + 0x118));
                    camY = 0;
                    *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
                    *(void **)(gs + 0x30) = player;
                }
            } else {
                /* results / ending screen */
                *(s16 *)(gs + 0x64) = 0;
                *(s16 *)(gs + 0x66) = 0;
                player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x158, 1, 0);
                player = CreateResultsScreenEntity(player, s_p1InputSlot, gs[0x18E]);
                camY = 0;
                *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
                *(s16 *)(gs + 0x120) = 0;
                *(s16 *)(gs + 0x122) = 0;
                gs[0x12A] = 0;
                *(void **)(gs + 0x30) = player;
            }
        } else {
            /* MENU entity */
            *(s16 *)(gs + 0x64) = 0;
            *(s16 *)(gs + 0x66) = 0;
            player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x140, 1, 0);
            player = InitMenuEntity(player, (s32)(uintptr_t)s_p1InputSlot,
                        (s32)(uintptr_t)(gs + 0x171), gs[0x17B]);
            camY = 0;
            *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
            *(void **)(gs + 0x30) = player;
        }
    } else {
        /* FINN swamp-boat player */
        *(s16 *)(gs + 0x64) = 0;
        *(s16 *)(gs + 0x66) = -0x50;
        player = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x114, 1, 0);
        player = CreateFinnPlayerEntity(player, s_p1InputSlot,
                    *(s16 *)(gs + 0x116), *(s16 *)(gs + 0x118));
        camY = 0;
        *(s32 *)(player + 0x100) = *(s32 *)(gs + 0x140);
        *(s16 *)(gs + 0x120) = 0;
        *(s16 *)(gs + 0x122) = 0;
        gs[0x12A] = 0;
        *(void **)(gs + 0x30) = player;
    }

    /* ---- common tail ---- */
    SetCameraPositionDirect(gs, 0, camY);
    AddEntityToSortedRenderList(gs, player);
    *(void **)(gs + 0x2C) = player;               /* player_entity_alt   */
    LoadLevelSpriteAssets((s32)(uintptr_t)gs);

    if ((GetLevelFlags(ctx) & 1) != 0) {
        s32 sp = InitMenuSpriteEntity(AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x30, 1, 0));
        AddToZOrderList(gs, sp);
        AddToXPositionList(gs, *(s32 *)((u8 *)(uintptr_t)sp + 0x1C));
    }

    {
        u8 *hud = (u8 *)CreateMenuEntities(AllocateFromHeap((u8 *)g_pBlbHeapBase, 0xB0, 1, 0),
                                           *(void **)(gs + 0x140));
        *(void **)(gs + 0x14C) = hud;
        AddToZOrderList(gs, (s32)(uintptr_t)hud);
    }

    /* one-shot HUD event dispatch when the level has a real asset index */
    if (GetCurrentLevelAssetIndex(ctx) != 0) {
        u8 *hud = *(u8 **)(gs + 0x14C);
        s32 marker = (s32)*(s16 *)(hud + 0xA);
        if (marker != 0) {
            void (*fn)(void *, s32, s32, s32);
            s16 argOff = 0;
            s32 base;
            if (marker < 1) {
                fn = *(void (**)(void *, s32, s32, s32))(hud + 0xC);
            } else {
                s32 slot = marker * 8 + *(s32 *)(hud + *(s16 *)(hud + 0xC));
                argOff = (s16)*(s32 *)((u8 *)(uintptr_t)slot - 8);
                fn = *(void (**)(void *, s32, s32, s32))((u8 *)(uintptr_t)slot - 4);
            }
            base = (s32)*(s16 *)(hud + 8);
            if ((marker << 16) > 0) {
                base = argOff + base;
            }
            fn(hud + base, 0x1013, 2, 0);
        }
    }
}
