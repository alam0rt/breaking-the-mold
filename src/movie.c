#include "common.h"

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromCD);

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromBLBSectors);

INCLUDE_ASM("asm/nonmatchings/movie", DisplayLoadingScreen);

INCLUDE_ASM("asm/nonmatchings/movie", SetupMovieDisplay);

INCLUDE_ASM("asm/nonmatchings/movie", InitMovieStreamingBuffers);

INCLUDE_ASM("asm/nonmatchings/movie", InitMovieDecoder);

INCLUDE_ASM("asm/nonmatchings/movie", MovieFrameDecodeCallback);

INCLUDE_ASM("asm/nonmatchings/movie", StubMovieStreamCallback);

INCLUDE_ASM("asm/nonmatchings/movie", DecodeNextMovieFrame);

INCLUDE_ASM("asm/nonmatchings/movie", GetNextStreamSector);

INCLUDE_ASM("asm/nonmatchings/movie", DecodeStaticFrame);

INCLUDE_ASM("asm/nonmatchings/movie", WaitForDecodeBufferSync);

INCLUDE_ASM("asm/nonmatchings/movie", SeekAndStartCDRead);

INCLUDE_ASM("asm/nonmatchings/movie", SetMovieDisplayEnvironment);

