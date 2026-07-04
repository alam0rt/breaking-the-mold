#include "common.h"

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromCD);

INCLUDE_ASM("asm/nonmatchings/movie", PlayMovieFromBLBSectors);

INCLUDE_ASM("asm/nonmatchings/movie", DisplayLoadingScreen);

INCLUDE_ASM("asm/nonmatchings/movie", SetupMovieDisplay);

INCLUDE_ASM("asm/nonmatchings/movie", InitMovieStreamingBuffers);

extern void DecDCTReset(s32 mode);
extern void DecDCToutCallback(void (*cb)(void));
extern s32 StSetRing(u8 *base, s32 size);
extern u8 *StSetStream(s32 mode, s32 chan, s32 nsec, void (*outCb)(void), void (*streamCb)(void));
extern void StubMovieStreamCallback(void);
extern void SeekAndStartCDRead(u8 *loc);

/* Tentative def to unlock gp_rel via maspsx --use-comm-section. */
u8 *MOVIE_STREAM_BUFFER_BASE asm("D_800A5A44");

void InitMovieDecoder(u8 *loc, void (*outCb)(void)) {
    DecDCTReset(0);
    DecDCToutCallback(outCb);
    StSetRing(MOVIE_STREAM_BUFFER_BASE + 0x67000, 0x100);
    StSetStream(1, 1, -1, 0, StubMovieStreamCallback);
    SeekAndStartCDRead(loc);
}

INCLUDE_ASM("asm/nonmatchings/movie", MovieFrameDecodeCallback);

void StubMovieStreamCallback(void) {
}

INCLUDE_ASM("asm/nonmatchings/movie", DecodeNextMovieFrame);

INCLUDE_ASM("asm/nonmatchings/movie", GetNextStreamSector);

INCLUDE_ASM("asm/nonmatchings/movie", DecodeStaticFrame);

INCLUDE_ASM("asm/nonmatchings/movie", WaitForDecodeBufferSync);

extern s32 CdControl(s32 cmd, u8 *param, u8 *result);
extern s32 CdRead2(s32 mode);
extern void VSync(s32 mode);

void SeekAndStartCDRead(u8 *loc) {
    u8 mode = 0x80;
    do {
        while (CdControl(2, loc, 0) == 0);
        while (CdControl(0xE, &mode, 0) == 0);
        VSync(3);
    } while (CdRead2(0x1E0) == 0);
}

INCLUDE_ASM("asm/nonmatchings/movie", SetMovieDisplayEnvironment);


/* .data island 0x8009B4CC..0x8009B4DC (16B, movie halfword table) migrated from asm. */
/* group island: 0-byte pad at 0x8009B4CC, 8 aliased symbol(s); anchor D_8009B4CC (16B). */
u8 D_8009B4CC[16] asm("D_8009B4CC") = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
asm(".globl D_8009B4CE\n.set D_8009B4CE, D_8009B4CC + 2\n.globl D_8009B4D0\n.set D_8009B4D0, D_8009B4CC + 4\n.globl D_8009B4D2\n.set D_8009B4D2, D_8009B4CC + 6\n.globl D_8009B4D4\n.set D_8009B4D4, D_8009B4CC + 8\n.globl D_8009B4D6\n.set D_8009B4D6, D_8009B4CC + 10\n.globl D_8009B4D8\n.set D_8009B4D8, D_8009B4CC + 12\n.globl D_8009B4DA\n.set D_8009B4DA, D_8009B4CC + 14\n");
