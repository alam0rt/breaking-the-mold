#include "common.h"

#ifdef NON_MATCHING
typedef s32 M2C_UNK;

M2C_UNK func_8008E7FC();                            /* extern */
M2C_UNK func_8008FA60(s32 *);                       /* extern */
extern s32 D_8009D1FC;
extern s16 D_8009D200;
extern s16 D_8009D202;
extern s16 D_8009D20C;
extern s32 D_8009D21C;
extern s32 D_8009D220;
extern s32 D_8009D224;
extern s16 D_8009D228;
extern s16 D_8009D22A;
extern s16 D_8009D22C;
extern s16 D_8009D22E;
extern s16 D_8009D230;
extern s8 D_800A6088;

void func_8007BFB8(void) {
    s32 sp10;
    s16 sp14;
    s16 sp16;
    s16 sp20;
    s16 sp22;
    s32 sp28;

    func_8008E7FC();
    sp10 = 0x2C3;
    sp14 = 0x3FFF;
    sp16 = 0x3FFF;
    sp20 = 0x7FFF;
    sp22 = 0x7FFF;
    sp28 = 1;
    func_8008FA60(&sp10);
    D_8009D1FC = 0xFF93;
    D_8009D20C = 0x800;
    D_8009D224 = 3;
    D_8009D200 = 0x3FFF;
    D_8009D202 = 0x3FFF;
    D_8009D21C = 1;
    D_8009D220 = 1;
    D_8009D228 = 0;
    D_8009D22A = 0;
    D_8009D22C = 0;
    D_8009D22E = 0;
    D_8009D230 = 0xF;
    D_800A6088 = 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/SetupSound", func_8007BFB8);
#endif
