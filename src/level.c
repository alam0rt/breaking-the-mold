#include "common.h"
#include "Game/level_data_context.h"

extern s32 strcmp(const char *s1, const char *s2);
extern s32 LoadAssetContainer(LevelDataContext *ctx, s32 arg1, s32 arg2);

INCLUDE_ASM("asm/nonmatchings/level", InitLevelDataContext);

INCLUDE_ASM("asm/nonmatchings/level", ResetGameStateInputAndContext);

INCLUDE_ASM("asm/nonmatchings/level", ClearContextOffsets68to7C);

INCLUDE_ASM("asm/nonmatchings/level", ClearLevelDataContext);

INCLUDE_ASM("asm/nonmatchings/level", AdvancePlaybackSequence);

INCLUDE_ASM("asm/nonmatchings/level", SetSequenceIndexByMode);

INCLUDE_ASM("asm/nonmatchings/level", SeekToLevelInSequence);

INCLUDE_ASM("asm/nonmatchings/level", FindSaveSlotByName);

INCLUDE_ASM("asm/nonmatchings/level", AdvanceLevelSequence);

INCLUDE_ASM("asm/nonmatchings/level", PeekNextPlaybackMode);

INCLUDE_ASM("asm/nonmatchings/level", GetPrimaryBufferSize);

INCLUDE_ASM("asm/nonmatchings/level", LevelDataParser);

INCLUDE_ASM("asm/nonmatchings/level", LoadLevelByWorldIndex);

INCLUDE_ASM("asm/nonmatchings/level", LoadLevelByWorldId);
