#include "common.h"

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromCD);

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromBLBSectors);

INCLUDE_ASM("asm/nonmatchings/movie", DisplayLoadingScreen);

INCLUDE_ASM("asm/nonmatchings/movie", SetupMovieDisplay);

INCLUDE_ASM("asm/nonmatchings/movie", InitMovieStreamingBuffers);

INCLUDE_ASM("asm/nonmatchings/movie", InitMovieDecoder);

INCLUDE_ASM("asm/nonmatchings/movie", MovieFrameDecodeCallback);

void StubMovieStreamCallback(void) {
}

INCLUDE_ASM("asm/nonmatchings/movie", DecodeNextMovieFrame);

INCLUDE_ASM("asm/nonmatchings/movie", GetNextStreamSector);

INCLUDE_ASM("asm/nonmatchings/movie", DecodeStaticFrame);

INCLUDE_ASM("asm/nonmatchings/movie", WaitForDecodeBufferSync);

extern s32 CdControl(s32 cmd, void *param, void *result);
extern s32 CdRead2(s32 mode);
extern void VSync(s32 mode);

void SeekAndStartCDRead(void *loc) {
    u8 mode = 0x80;
    do {
        while (CdControl(2, loc, 0) == 0);
        while (CdControl(0xE, &mode, 0) == 0);
        VSync(3);
    } while (CdRead2(0x1E0) == 0);
}

INCLUDE_ASM("asm/nonmatchings/movie", SetMovieDisplayEnvironment);

