#include "common.h"

INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_Stub);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_ReadSectors);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_WaitReadComplete);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_SetupPosition);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Start);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Stop);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Pause);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Resume);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Update);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdSeekWithRetry);
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdResetFilter);
