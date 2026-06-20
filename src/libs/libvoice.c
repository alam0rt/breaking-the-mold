#include "common.h"

extern u8 *SPU_REGISTER_BASE[] asm("D_800A55F4");
extern void spu_Fw1ts(void);

/* Per-voice SPU register block (16 bytes/voice, 24 voices total).
 * Only the fields touched by this file are named -- the SPU exposes more
 * (addr/loop/ADSR) in the remaining 10 bytes. */
typedef struct {
    /* 0x00 */ u16 vol_l;
    /* 0x02 */ u16 vol_r;
    /* 0x04 */ u16 pitch;
    /* 0x06 */ u8  _pad06[0x0A];
} SpuVoiceRegisters;  /* 0x10 */

void SpuSetVoiceVolume(s32 voice, s16 volL, s16 volR) {
    SpuVoiceRegisters *regs;

    volL &= 0x7FFF;
    volR &= 0x7FFF;
    regs = (SpuVoiceRegisters *)((voice << 4) + (s32)SPU_REGISTER_BASE[0]);

    regs->vol_l = volL;
    regs->vol_r = volR;
    /* @hack: memory fence keeps both SPU register stores before spu_Fw1ts() (matches PSY-Q libvoice; the do {} while (0) idiom is NOT a drop-in here -- without the asm fence the compiler fills the jal delay slot with the second store, shifting the function by 4 bytes). */
    __asm__ volatile ("" : : : "memory");
    spu_Fw1ts();
}

void SpuSetVoicePitch(s32 voice, u16 pitch) {
    SpuVoiceRegisters *regs;

    regs = (SpuVoiceRegisters *)((voice << 4) + (s32)SPU_REGISTER_BASE[0]);

    regs->pitch = pitch;
    /* @hack: memory fence keeps pitch store before spu_Fw1ts() (matches PSY-Q libvoice; do {} while (0) lets the compiler fold the store into the jal delay slot, breaking match). */
    __asm__ volatile ("" : : : "memory");
    spu_Fw1ts();
}

void SpuGetVoicePitch(s32 voice, u16 *pitch) {
    SpuVoiceRegisters *regs;

    regs = (SpuVoiceRegisters *)((voice << 4) + (s32)SPU_REGISTER_BASE[0]);

    *pitch = regs->pitch;
}

void SpuSetCommonCDVolume(s16 left, s16 right) {
    /* CD-volume registers live at SPU_BASE+0x1B0 / +0x1B2; pointer-aliased
     * here rather than promoted into a full SpuRegisterBlock since this is
     * the only file that touches them. */
    s16 *cd_vol = (s16 *)(SPU_REGISTER_BASE[0] + 0x1B0);

    cd_vol[0] = left;
    cd_vol[1] = right;
    /* @hack: memory fence pins both CD-volume register stores at function end (matches PSY-Q libvoice; without it the second sh is folded into the jr delay slot). */
    __asm__ volatile ("" : : : "memory");
}
