#include "common.h"

INCLUDE_ASM("asm/nonmatchings/results", InitHUDDigitDisplay);

INCLUDE_ASM("asm/nonmatchings/results", HUDDigitDisplayTickCallback);

INCLUDE_ASM("asm/nonmatchings/results", CreateResultsScreenEntity);


/* .data island 0x8009CB7C..0x8009CBAC (48B, HUD digit position table) migrated from asm. */
/* 12 {x,y} screen positions (two rows of 6 digits); the +0/+2 aliases expose
 * the x and y columns, read with a 4-byte stride by the HUD digit code. */
typedef struct {
    s16 x;
    s16 y;
} HUDDigitPos;
HUDDigitPos D_8009CB7C[12] asm("D_8009CB7C") = {
    {0x00C0, 0x0046}, {0x00D1, 0x0046}, {0x00E3, 0x0045},
    {0x00F5, 0x0045}, {0x0106, 0x0046}, {0x0118, 0x0047},
    {0x00C0, 0x0065}, {0x00D2, 0x0065}, {0x00E4, 0x0065},
    {0x00F4, 0x0064}, {0x0107, 0x0066}, {0x0119, 0x0066},
};
asm(".globl D_8009CB7E\n.set D_8009CB7E, D_8009CB7C + 2\n");
