#include "common.h"

INCLUDE_ASM("asm/nonmatchings/gamecd", LoadGameAssetLocations);

INCLUDE_ASM("asm/nonmatchings/gamecd", func_80038B98);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdBLB_ReadSectors);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdReadFileSync);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdSetModeAndSeek);

INCLUDE_ASM("asm/nonmatchings/gamecd", PlayCDAudioTrack);

INCLUDE_ASM("asm/nonmatchings/gamecd", StopCDAudio);

INCLUDE_ASM("asm/nonmatchings/gamecd", PauseCDAudio);

INCLUDE_ASM("asm/nonmatchings/gamecd", ResumeCDAudio);

INCLUDE_ASM("asm/nonmatchings/gamecd", ProcessCDStreamState);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdSeekWithRetry);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdPausePlayback);

