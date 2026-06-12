#include "common.h"

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", GetFrameReadyFlag);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", TriggerBufferSwapIfReady);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", SetVideoModePAL);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", SsUtReverbOn);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013268);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013490);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800134B8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013500);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001352C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013554);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800136C8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013718);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013760);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800137B0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013800);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013850);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800138A0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800138F0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013AB0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013B1C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013D10);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013F50);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014134);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014278);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800143A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800143F0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800145A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014854);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014928);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014968);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800149E8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014A9C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014CF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015074);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800150C4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015134);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015424);

/* Empty function. NOTE: cannot live here as decompiled C - gcc 2.7.2 at -O2
 * defers tiny inline-candidate functions to end-of-file, which breaks the
 * address-order layout this file must preserve (verified: the 8-byte body
 * relocated to 0x8001A0B8 and shifted everything after 0x80015434). */
INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015434);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001543C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001546C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015614);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800156A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015764);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015E0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015EB4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001601C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016470);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001652C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800165A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016AC8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016FE8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001713C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017540);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017A3C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017AF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017D0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018110);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800181FC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001889C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800188E0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BD8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BE0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BE8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BF4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C18);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C24);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C30);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C38);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C40);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C4C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C58);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C60);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C6C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C78);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C80);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C88);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C94);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C9C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CC0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CD0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CD8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D00);

/* Empty function. See func_80015434 note re: -O2 deferred output. */
INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D4C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D54);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D94);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018DDC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800191DC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019238);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019364);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800193D4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019474);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800194F4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001954C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019558);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800195B0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001960C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019618);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001963C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019650);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019678);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800196D8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019700);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019748);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019790);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019864);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001991C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019A14);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019CF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019D74);

/* =============================================================================
 * CLUT effect descriptors (palette cycle / color lerp)
 *
 * Decompiled functions live at the END of this file: gcc 2.7.2 at -O2
 * buffers all compiled function bodies and emits them after the streamed
 * INCLUDE_ASM blocks, so only a contiguous tail of this module can be
 * decompiled in-place. Decompile backwards from the end.
 * ============================================================================= */

/* Tick-slot install pattern: {markerLo=0, markerHi=-1, fn} written as a
 * local struct then copied into the descriptor head as two words
 * (matches the FSM marker encoding in include/Game/entity.h).
 * The local is wrapped in PaddedTickSlot: the original stack frames have
 * the slot at sp+4 with a 4-byte hole below, which only reproduces when
 * the slot sits at offset 4 of a larger local. */
typedef struct {
    /* 0x00 */ s16 markerLo;
    /* 0x02 */ s16 markerHi;
    /* 0x04 */ void (*fn)();
} TickSlot;

typedef struct {
    /* 0x00 */ s32 pad;
    /* 0x04 */ TickSlot t;
} PaddedTickSlot;

/* 256-entry 15-bit CLUT (one full palette). Halfword alignment forces the
 * compiler's runtime aligned/unaligned dual-path block copy, matching the
 * original. */
typedef struct {
    u16 entries[256];
} CLUT256;

/* Palette effect descriptor (palette cycling and color lerp).
 * Field offsets verified against func_80019F2C/func_80019F88 asm. */
typedef struct {
    /* 0x00 */ TickSlot tick;        /* Per-frame tick callback slot */
    /* 0x08 */ u8       pad08[0x14];
    /* 0x1C */ CLUT256 *srcClut;     /* Source CLUT (0 = effect disabled) */
    /* 0x20 */ CLUT256 *workBuf;     /* Working CLUT buffer (lazily allocated) */
    /* 0x24 */ u32      targetClut;  /* Lerp target CLUT */
    /* 0x28 */ u8       pad28[6];
    /* 0x2E */ u16      totalFrames; /* Lerp duration in frames */
    /* 0x30 */ u16      currentFrame;
    /* 0x32 */ u16      unk32;
    /* 0x34 */ u8       pad34;
    /* 0x35 */ u8       param35;
    /* 0x36 */ u8       param36;
    /* 0x37 */ u8       param37;
    /* 0x38 */ u8       param38;
    /* 0x39 */ u8       param39;     /* Mirror of param38 */
} CLUTEffectDesc;

void func_8001991C(); /* palette-cycle tick callback (asm above) */
void func_80019A14(); /* color-lerp tick callback (asm above) */
s32 func_800143F0(s32 heap, s32 size, s32 count, u8 flag); /* AllocateFromHeap */
extern s32 D_800A5954[]; /* g_pBlbHeapBase (array decl avoids gp-rel access) */

/* Install the CLUT palette-cycle tick callback with its parameters.
 * Ghidra: SetTexturePageParams @ 0x80019F2C. */
void func_80019F2C(CLUTEffectDesc *desc, u8 arg1, u8 arg2, u8 arg3, u8 arg4) {
    PaddedTickSlot u;

    if (desc->srcClut != 0) {
        desc->param35 = arg2;
        desc->param36 = arg3;
        desc->param37 = arg4;
        desc->param38 = arg1;
        desc->param39 = arg1;
        u.t.markerLo = 0;
        u.t.markerHi = -1;
        u.t.fn = func_8001991C;
        desc->tick = u.t;
    }
}

/* Init a CLUT color-lerp effect: store lerp parameters, install the lerp
 * tick callback, lazily allocate the 512-byte working CLUT and copy the
 * source palette into it.
 * Ghidra: InitCLUTColorLerpEffect @ 0x80019F88. */
void func_80019F88(CLUTEffectDesc *desc, u32 targetClut, u16 numFrames, u8 flag, u8 channels, u8 easing) {
    PaddedTickSlot u;

    if (desc->srcClut != 0) {
        desc->targetClut = targetClut;
        desc->totalFrames = numFrames;
        desc->currentFrame = 0;
        desc->unk32 = 0;
        desc->param35 = channels;
        desc->param36 = easing;
        desc->param38 = flag;
        desc->param39 = flag;
        u.t.markerLo = 0;
        u.t.markerHi = -1;
        u.t.fn = func_80019A14;
        desc->tick = u.t;
        if (desc->workBuf == 0) {
            desc->workBuf = (CLUT256 *)func_800143F0(D_800A5954[0], 2, 0x100, 0);
        }
        *desc->workBuf = *desc->srcClut;
    }
}
