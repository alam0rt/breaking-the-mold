#include "common.h"

INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdBLB_Stub);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdBLB_ReadSectors);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdBLB_WaitReadComplete);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_SetupPosition);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_Start);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_Stop);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_Pause);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_Resume);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdMusic_Update);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdSeekWithRetry);
INCLUDE_ASM("asm/pal/nonmatchings/cd/CdBLB", CdResetFilter);
