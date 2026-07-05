/* =============================================================================
 * MenuInputHandler.c  --  PC-port menu navigation input dispatcher
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c MenuInputHandler (0x800779A8+,
 * INCLUDE_ASM in src/passwd.c). Reads the menu entity's InputState and drives
 * cursor movement + widget actions by invoking the current menu item's widget
 * vtable callbacks:
 *   +0x24 highlight / +0x2c unhighlight (arg offsets +0x20 / +0x28)
 *   +0x34 left-adjust(value) / +0x3c right-adjust (arg +0x30 / +0x38)
 *   +0x44 fast-up / +0x4c fast-down (arg +0x40 / +0x48)
 *   +0x54 confirm/action(code) (arg +0x50)
 *
 * Menu entity layout: +0x100 InputState*, +0x104 item-pointer array (stride 4),
 * +0x12c item count (u8), +0x12d cursor index (u8), +0x131 auto-repeat counter.
 * Input words are read from InputState: +0x00 buttons_held, +0x02 buttons_pressed.
 *
 * With no input every masked branch is false and the function returns without
 * dispatching, so the idle title screen exercises only the early-out paths.
 *
 * Control flow uses function-scope gotos matching the export label targets;
 * all locals are declared up front.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

typedef void (*MenuFn1)(int base);
typedef void (*MenuFn2)(int base, int arg);

extern void PlaySoundEffect(u32 id, u16 vol, u8 flags);

#define VCALL1(vt, off, argbase, argoff) \
    ((MenuFn1)*(void **)((u8 *)(intptr_t)(vt) + (off)))( \
        (argbase) + *(s16 *)((u8 *)(intptr_t)(vt) + (argoff)))
#define VCALL2(vt, off, argbase, argoff, a2) \
    ((MenuFn2)*(void **)((u8 *)(intptr_t)(vt) + (off)))( \
        (argbase) + *(s16 *)((u8 *)(intptr_t)(vt) + (argoff)), (a2))

void MenuInputHandler(int param_1) {
    u8 *base = (u8 *)(intptr_t)param_1;
    u16 uVar1, uVar2;
    u8  bVar3 = 0;
    s32 item, vtable;
    s32 vtOffset;
    void *pcVar5;
    u32 *input32;
    u32 uVar8;

    if (*(u8 *)(base + 0x12c) == 0) {
        return;
    }

    uVar1 = *(u16 *)(*(s32 *)(base + 0x100) + 2);      /* buttons_pressed */
    if ((uVar1 & 0x4000) == 0) {
        if ((uVar1 & 0x1000) != 0) {
            if (*(u8 *)(base + 0x12d) == 0) {
                item = *(s32 *)(base + 0x104);
                vtable = *(s32 *)((intptr_t)item + 0x18);
                VCALL1(vtable, 0x2c, item, 0x28);
                bVar3 = *(char *)(base + 0x12c) - 1;
            } else {
                item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
                vtable = *(s32 *)((intptr_t)item + 0x18);
                VCALL1(vtable, 0x2c, item, 0x28);
                bVar3 = *(u8 *)(base + 0x12d) - 1;
            }
            goto set_cursor;
        }
    } else {
        s32 idx4 = (u32)*(u8 *)(base + 0x12d) * 4;
        if ((u32)*(u8 *)(base + 0x12d) == (u32)(*(u8 *)(base + 0x12c) - 1)) {
            item = *(s32 *)((intptr_t)base + idx4 + 0x104);
            vtable = *(s32 *)((intptr_t)item + 0x18);
            VCALL1(vtable, 0x2c, item, 0x28);
            item = *(s32 *)(base + 0x104);
            *(u8 *)(base + 0x12d) = 0;
        } else {
            item = *(s32 *)((intptr_t)base + idx4 + 0x104);
            vtable = *(s32 *)((intptr_t)item + 0x18);
            VCALL1(vtable, 0x2c, item, 0x28);
            bVar3 = *(u8 *)(base + 0x12d) + 1;
set_cursor:
            *(u8 *)(base + 0x12d) = bVar3;
            item = *(s32 *)((u32)bVar3 * 4 + (intptr_t)base + 0x104);
        }
        vtable = *(s32 *)((intptr_t)item + 0x18);
        VCALL1(vtable, 0x24, item, 0x20);
        PlaySoundEffect(0x646c2cc0, 0xa0, 0);
    }

    uVar1 = ((u16 *)(intptr_t)*(s32 *)(base + 0x100))[1];   /* buttons_pressed */
    if ((uVar1 & 0x8000) == 0) {
        uVar2 = *(u16 *)(intptr_t)*(s32 *)(base + 0x100);   /* buttons_held */
        if ((uVar2 & 0x8000) == 0) {
            if (((uVar1 & 0x2000) != 0) ||
                (((uVar2 & 0x2000) != 0) &&
                 (bVar3 = *(char *)(base + 0x131) + 1,
                  *(u8 *)(base + 0x131) = bVar3, 10 < bVar3))) {
                item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
                vtable = *(s32 *)((intptr_t)item + 0x18);
                vtOffset = *(s16 *)((intptr_t)vtable + 0x40);
                pcVar5 = *(void **)((intptr_t)vtable + 0x44);
                goto dispatch_updown;
            }
        } else {
            bVar3 = *(char *)(base + 0x131) + 1;
            *(u8 *)(base + 0x131) = bVar3;
            if (10 < bVar3) {
                item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
                vtable = *(s32 *)((intptr_t)item + 0x18);
                vtOffset = *(s16 *)((intptr_t)vtable + 0x48);
                pcVar5 = *(void **)((intptr_t)vtable + 0x4c);
                goto dispatch_updown;
            }
        }
    } else {
        item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
        vtable = *(s32 *)((intptr_t)item + 0x18);
        vtOffset = *(s16 *)((intptr_t)vtable + 0x48);
        pcVar5 = *(void **)((intptr_t)vtable + 0x4c);
dispatch_updown:
        ((MenuFn1)pcVar5)(item + vtOffset);
        *(u8 *)(base + 0x131) = 0;
    }

    input32 = (u32 *)(intptr_t)*(s32 *)(base + 0x100);
    if ((*input32 & 0x8400000) == 0) {
        if ((*input32 & 0x100000) != 0) {
            item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
            vtable = *(s32 *)((intptr_t)item + 0x18);
            VCALL1(vtable, 0x3c, item, 0x38);
            goto after_leftright;
        }
    } else {
        item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
        vtable = *(s32 *)((intptr_t)item + 0x18);
        VCALL2(vtable, 0x34, item, 0x30, (*(u16 *)((intptr_t)input32 + 2) >> 0xb) & 1);
after_leftright:
        uVar8 = 2;
        if ((*(u16 *)(*(s32 *)(base + 0x100) + 2) & 0x10) != 0) {
            item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
            vtable = *(s32 *)((intptr_t)item + 0x18);
            goto dispatch_action;
        }
    }

    uVar1 = *(u16 *)(*(s32 *)(base + 0x100) + 2);   /* buttons_pressed */
    uVar8 = 1;
    if ((uVar1 & 0x80) == 0) {
        uVar8 = 3;
        if ((uVar1 & 0x20) == 0) {
            uVar8 = 0;
            if ((uVar1 & 0x40) == 0) {
                uVar8 = 4;
                if ((uVar1 & 4) == 0) {
                    uVar8 = 5;
                    if ((uVar1 & 1) == 0) {
                        uVar8 = 6;
                        if ((uVar1 & 8) == 0) {
                            uVar8 = 7;
                            if ((uVar1 & 2) == 0) {
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    item = *(s32 *)((u32)*(u8 *)(base + 0x12d) * 4 + (intptr_t)base + 0x104);
    vtable = *(s32 *)((intptr_t)item + 0x18);
dispatch_action:
    VCALL2(vtable, 0x54, item, 0x50, uVar8);
}
