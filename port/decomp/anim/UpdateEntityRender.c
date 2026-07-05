/* =============================================================================
 * UpdateEntityRender.c  --  PC-port sprite render/decode driver
 * =============================================================================
 * Faithful transcription of asm/nonmatchings/anim/UpdateEntityRender.s
 * (0x8001D988, INCLUDE_ASM in src/gfx.c) cross-checked against
 * export/SLES_010.90.c UpdateEntityRender. This is the per-entity render step:
 * it resolves the entity's parallax-scaled screen X/Y (running the move-marker
 * FSM per axis), publishes scale + shadow coordinates into the sprite context,
 * then drives the sprite frame decode / texture-CLUT upload state machine.
 *
 * All entity fields are addressed by explicit byte offset off a u8* to match the
 * MIPS exactly; the export's Entity/entity[1] member names are NOT reused (the
 * "entity[1].X" region aliases sprite/anim decode state, not a second Entity).
 *
 * Move-marker FSM (per axis, base = entity itself):
 *   markerHi == 0 -> coordinate is the raw world coordinate.
 *   markerHi  < 0 -> callback = *(fn*)(e + cbOff), base = e + markerLo.
 *   markerHi  > 0 -> slot table at e + *(s16*)(e + cbOff); slot[hi].arg adds to
 *                    markerLo, slot[hi].fn is the callback. base = e + off.
 * The camera (parallax) subtract is applied uniformly at each call site.
 *
 * Verified entity byte offsets (base Entity, 0x80 stride; extended region = +0x80):
 *   0x24 markerX.lo / 0x26 markerX.hi / 0x28 callbackX   (X FSM)
 *   0x2C markerY.lo / 0x2E markerY.hi / 0x30 callbackY   (Y FSM)
 *   0x34 spriteContext*  0x38 renderOffX  0x3A renderOffY
 *   0x3C renderW         0x3E renderH
 *   0x50 scaleRender     0x54 scaleRender2  0x60 scaleParX  0x64 scaleParY
 *   0x68 worldX          0x6A worldY        0x72 shadowFlag
 *   0x74 facing (u8)     0x75 flipY (u8)    0x76 textureDirty (u8)
 *   0x78 pFrameTable*
 *   0x88 frameCount(u16) 0x8A eventByte     0x8C setupCtx*  0x90 spriteAsset*
 *   0xB0 pixelBuffer*    0xCC/0xCE render-key(32b) 0xD0 render-key-cache(32b)
 *   0xDA targetFrame(u16) 0xEA curFrame(u16) 0xEC frameDiv(u16) 0xEE rowBudget(u16)
 *   0xF6 clutDirty(u8)   0xF7 staticSprite(u8)
 *   0xF8 restartFlag  0xF9 decodeDone/bVar4  0xFA facingCache  0xFB flipYCache
 *   0xFC bufClearedFlag  0xFD forceVisibleFlag
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

typedef s32 (*MoveFn)(void *base, s32 coord);

extern u32  IsEntityOffScreen(void *entity);
extern void SetupSpriteFromFrame(void *setupCtx, void *spriteContext, u32 frameIndex);
extern u32  DecodeRLESpriteChecked(void *ctx, void *src, u16 size);
extern void RenderSprite(void *ctx, u32 frameIdx, void *dst, s32 rowStride, s32 flagH, s32 flagV);
extern void GetFrameMetadata(void *ctx, void *src, u32 frameIdx, void *dst, s32 rowStride,
                             u8 flagH, u8 flagV);
extern void memset_entrypoint(void *dst, int count, int fill);

/* Raw move-marker FSM: returns the world coordinate (markerHi == 0) or the
 * callback-produced coordinate. Camera subtract is applied by the caller. */
static s32 coordFsm(u8 *e, int hiOff, int cbOff, int loOff, s16 world) {
    s16 hi = *(s16 *)(e + hiOff);
    MoveFn fn;
    s16 arg = 0;
    s32 off;

    if (hi == 0) {
        return (s32)world;
    }
    if (hi < 1) {
        fn = *(MoveFn *)(e + cbOff);
    } else {
        s32 slot = (s32)hi * 8 + *(s32 *)(e + *(s16 *)(e + cbOff));
        arg = (s16)*(u32 *)((u8 *)(intptr_t)slot - 8);
        fn = *(MoveFn *)((u8 *)(intptr_t)slot - 4);
    }
    off = *(s16 *)(e + loOff);
    if (hi > 0) {
        off = (s32)arg + off;
    }
    return fn(e + off, (s32)world);
}

/* Signed fixed-point extract: (value * scale) >> 16, arithmetic shift.
 * Matches the MIPS `mult` + `mflo` + `sra` used for the render-scale terms. */
static s32 scaleShift(s32 value, s32 scale) {
    return (value * scale) >> 16;
}

void UpdateEntityRender(void *entityPtr) {
    u8 *e = (u8 *)entityPtr;
    u8 *sc;
    s16 camX;   /* parallax-scaled camera_x (s0 in asm) */
    s16 camY;   /* parallax-scaled camera_y (s2 in asm) */
    s32 coord;
    u16 roundW;
    s16 flipOff;
    u8  bVar1 = 0;
    u8  bVar4 = 0;

    /* ---- parallax-scaled camera (logical shift on the scaled path) ---- */
    if (*(s32 *)(e + 0x60) == 0x10000) {
        camX = g_pGameState->camera_x;
    } else {
        camX = (s16)((u32)((s32)g_pGameState->camera_x * *(s32 *)(e + 0x60)) >> 0x10);
    }
    if (*(s32 *)(e + 0x64) == 0x10000) {
        camY = g_pGameState->camera_y;
    } else {
        camY = (s16)((u32)((s32)g_pGameState->camera_y * *(s32 *)(e + 0x64)) >> 0x10);
    }

    sc = *(u8 **)(e + 0x34);
    if (sc == (u8 *)0) {
        return;
    }

    /* ---- screenX ---- */
    if (*(s32 *)(e + 0x50) == 0x10000) {                 /* scaleRender == 1.0 */
        if (*(u8 *)(e + 0x74) == 0) {                    /* facing == 0 */
            coord = coordFsm(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68)) - camX;
            *(s16 *)(sc + 0x00) = (s16)(*(s16 *)(e + 0x38) + coord);
        } else {                                         /* facing != 0 (mirror) */
            coord = coordFsm(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68)) - camX;
            coord = (coord - (s32)(((u16)*(u16 *)(e + 0x3C) + 3) & 0xFFFC)) - *(s16 *)(e + 0x38);
            *(s16 *)(sc + 0x00) = (s16)(coord + 1);
        }
    } else {                                             /* scaled */
        if (*(u8 *)(e + 0x74) != 0) {                    /* facing != 0 */
            coord = coordFsm(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68));
            coord = ((coord - camX)
                     - scaleShift((((s32)*(s16 *)(e + 0x3C) + 3) & ~3), *(s32 *)(e + 0x50)))
                    - scaleShift((s32)*(s16 *)(e + 0x38), *(s32 *)(e + 0x50));
            *(s16 *)(sc + 0x00) = (s16)(coord + 1);
        } else {                                         /* facing == 0 */
            coord = coordFsm(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68));
            *(s16 *)(sc + 0x00) =
                (s16)((coord - camX) + scaleShift((s32)*(s16 *)(e + 0x38), *(s32 *)(e + 0x50)));
        }
    }

    /* ---- screenY ---- */
    if (*(s32 *)(e + 0x54) == 0x10000) {                 /* scaleRender2 == 1.0 */
        if (*(u8 *)(e + 0x75) == 0) {                    /* flipY == 0 */
            coord = coordFsm(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A)) - camY;
            *(s16 *)(sc + 0x02) = (s16)(*(s16 *)(e + 0x3A) + coord);
        } else {                                         /* flipY != 0 */
            coord = coordFsm(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A)) - camY;
            coord = (coord - *(s16 *)(e + 0x3E)) - *(s16 *)(e + 0x3A);
            *(s16 *)(sc + 0x02) = (s16)(coord + 1);
        }
    } else {                                             /* scaled */
        if (*(u8 *)(e + 0x75) != 0) {                    /* flipY != 0 */
            coord = coordFsm(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A));
            coord = ((coord - camY)
                     - scaleShift((s32)*(s16 *)(e + 0x3E), *(s32 *)(e + 0x54)))
                    - scaleShift((s32)*(s16 *)(e + 0x3A), *(s32 *)(e + 0x54));
            *(s16 *)(sc + 0x02) = (s16)(coord + 1);
        } else {                                         /* flipY == 0 */
            coord = coordFsm(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A));
            *(s16 *)(sc + 0x02) =
                (s16)((coord - camY) + scaleShift((s32)*(s16 *)(e + 0x3A), *(s32 *)(e + 0x54)));
        }
    }

    /* ---- publish scale + shadow coordinates into the sprite context ---- */
    *(s32 *)(sc + 0x1C) = *(s32 *)(e + 0x50);            /* scaleRender  */
    *(s32 *)(sc + 0x20) = *(s32 *)(e + 0x54);            /* scaleRender2 */
    *(s16 *)(sc + 0x28) = (s16)*(u16 *)(e + 0x72);       /* shadowFlag   */
    if (*(u16 *)(e + 0x72) != 0) {
        s32 sx = coordFsm(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68)) - camX;
        s32 sy = coordFsm(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A)) - camY;
        sc = *(u8 **)(e + 0x34);
        *(s16 *)(sc + 0x2A) = (s16)sx;
        *(s16 *)(sc + 0x2C) = (s16)sy;
    }

    /* ---- static-sprite (pre-decoded) path: CLUT re-setup only ---- */
    if (*(u8 *)(e + 0xF7) != 0) {
        if (*(u8 *)(e + 0x76) == 0) {                    /* textureDirty */
            return;
        }
        if (*(u8 *)(e + 0xFD) == 0 && (IsEntityOffScreen(e) & 0xFF) != 0) {
            return;
        }
        SetupSpriteFromFrame(e + 0x8C, *(u8 **)(e + 0x34), (u32)*(u16 *)(e + 0xDA));
        if (*(u8 *)(e + 0xF6) != 0) {                    /* clutDirty */
            *(u8 *)(*(u8 **)(e + 0x34) + 0x37) = *(u8 *)(*(u8 **)(e + 0x8C) + 0x14);
        }
        *(u8 *)(e + 0x76) = 0;
        *(u8 *)(e + 0xF8) = 0;
        return;
    }

    /* ---- dynamic decode path ---- */
    if (*(u8 **)(e + 0xB0) == (u8 *)0) {                 /* pixel buffer */
        return;
    }
    if (*(u8 *)(e + 0xFD) == 0 && (IsEntityOffScreen(e) & 0xFF) != 0) {
        return;
    }

    if (*(u8 *)(e + 0x76) == 0) {                        /* textureDirty == 0: continue in-progress decode */
        if (*(u8 *)(e + 0xF9) == 0) {
            return;
        }
        if (*(u8 *)(e + 0xFC) == 0) {
            u8 *ft  = *(u8 **)(e + 0x78);
            u16 idx = *(u16 *)(e + 0xEA);
            s16 rw  = (s16)(((u16)*(u16 *)(ft + idx * 0x24 + 0x0A) + 3) & 0xFFFC);
            memset_entrypoint(*(u8 **)(e + 0xB0),
                              ((s32)rw * (s32)*(s16 *)(ft + idx * 0x24 + 0x0C)) & 0xFFFC, 0);
            *(u8 *)(e + 0xFC) = 1;
        }
        bVar4 = (u8)((DecodeRLESpriteChecked(e + 0x78, *(u8 **)(e + 0x90),
                                             *(u16 *)(e + 0xEE)) & 0xFF) == 0);
        goto LAB_8001e588;
    }

    /* textureDirty != 0 */
    {
        if (*(s32 *)(e + 0xD0) == *(s32 *)(e + 0xCC)             /* render key unchanged   */
            && *(u16 *)(e + 0xEA) == *(u16 *)(e + 0xDA)          /* frame index unchanged  */
            && *(u16 *)(e + 0xFA) == *(u16 *)(e + 0x74)) {       /* facing/flipY unchanged */
            /* same frame: keep advancing the current decode */
            if (*(u8 *)(e + 0xF9) != 0) {
                if (*(u8 *)(e + 0xFC) == 0) {
                    u8 *ft  = *(u8 **)(e + 0x78);
                    u16 idx = *(u16 *)(e + 0xEA);
                    s16 rw  = (s16)(((u16)*(u16 *)(ft + idx * 0x24 + 0x0A) + 3) & 0xFFFC);
                    memset_entrypoint(*(u8 **)(e + 0xB0),
                                      ((s32)rw * (s32)*(s16 *)(ft + idx * 0x24 + 0x0C)) & 0xFFFC, 0);
                    *(u8 *)(e + 0xFC) = 1;
                }
                DecodeRLESpriteChecked(e + 0x78, *(u8 **)(e + 0x90), 0xFFFF);
                bVar1 = *(u8 *)(e + 0xF6);
                *(u8 *)(e + 0xF9) = 0;
                goto code_r0x8001e3b4;
            }
            /* decodeDone == 0: fall through to frame advance */
        } else {
            /* frame changed: render the target frame fresh */
            roundW = (u16)((*(u16 *)(e + 0x3C) + 3) & 0xFFFC);
            if (*(u8 *)(e + 0x74) == 0) {
                flipOff = 0;
            } else {
                flipOff = (s16)(roundW - *(u16 *)(e + 0x3C));
            }
            memset_entrypoint(*(u8 **)(e + 0xB0),
                              ((s32)(s16)roundW * (s32)*(s16 *)(e + 0x3E)) & 0xFFFC, 0);
            RenderSprite(e + 0x78, (u32)*(u16 *)(e + 0xDA), *(u8 **)(e + 0xB0) + flipOff,
                         (s16)roundW, *(u8 *)(e + 0x74), *(u8 *)(e + 0x75));
            bVar1 = *(u8 *)(e + 0xF6);
        code_r0x8001e3b4:
            if (bVar1 != 0) {
                *(u8 *)(*(u8 **)(e + 0x34) + 0x37) = *(u8 *)(e + 0x8A);
            }
        }

        /* ---- advance to the next frame ---- */
        {
            s16 nextIdx = (s16)(*(u16 *)(e + 0xDA) + 1);
            u16 idx;
            u16 frameDiv;
            u8 *ft;
            u8 *frame;
            u16 roundW2;
            s16 flipOff2;

            *(u16 *)(e + 0xEA) = (u16)nextIdx;
            *(s32 *)(e + 0xD0) = *(s32 *)(e + 0xCC);
            if ((s32)*(u16 *)(e + 0x88) <= (s32)nextIdx) {
                *(u16 *)(e + 0xEA) = 0;
            }

            idx      = *(u16 *)(e + 0xEA);
            frameDiv = *(u16 *)(e + 0xEC);
            ft       = *(u8 **)(e + 0x78);
            frame    = ft + idx * 0x24;
            if (frameDiv == 0) {
                *(u16 *)(e + 0xEE) = *(u16 *)(frame + 0x0C);
            } else {
                *(u16 *)(e + 0xEE) = (s16)((s32)*(s16 *)(frame + 0x0C) / (s32)frameDiv);
            }
            if (*(u16 *)(e + 0xEE) < 5) {
                *(u16 *)(e + 0xEE) = 5;
            }

            *(u8 *)(e + 0xFC) = 0;
            *(u8 *)(e + 0xFA) = *(u8 *)(e + 0x74);
            *(u8 *)(e + 0xFB) = *(u8 *)(e + 0x75);

            roundW2 = (u16)((*(u16 *)(frame + 0x0A) + 3) & 0xFFFC);
            if (*(u8 *)(e + 0x74) == 0) {
                flipOff2 = 0;
            } else {
                flipOff2 = (s16)(roundW2 - *(u16 *)(frame + 0x0A));
            }
            GetFrameMetadata(e + 0x78, *(u8 **)(e + 0x90), (u32)*(u16 *)(e + 0xEA),
                             *(u8 **)(e + 0xB0) + flipOff2, (s16)roundW2,
                             *(u8 *)(e + 0xFA), *(u8 *)(e + 0xFB));
            bVar4 = 1;
        }
    }

LAB_8001e588:
    *(u8 *)(e + 0xF9) = bVar4;
    return;
}
