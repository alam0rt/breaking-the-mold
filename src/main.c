#include "common.h"
/* ============================================================================
 * main - Main entry point and game loop
 * Address: 0x80039140 | Size: 0x3A4
 * ============================================================================ */

#ifdef NON_MATCHING
// Non-matching decompiled code for reference
// Accesses to sdata globals will generate incorrect (non-GP-relative) code

extern void __main(void);
extern void SsUtReverbOn(s32);
/* ResetCallback declared in LIBETC.H */
extern void LoadGameAssetLocations(void);
extern void func_80013268(s32);
extern void PadInit(s32);
extern void InitGeom(void);
extern void SetDispMask(s32);
extern void FntLoad(s32, s32);
extern s32 FntOpen(s32, s32, s32, s32, s32, s32);
extern void SetDumpFnt(s32);
extern void initPlayerState(u8 *);
extern void func_8007CD34(void *, s32);
extern u8 GetLevelCount(void *);
extern char *getLevelName(void *, u8);
extern u8 GetAssetCount(void *);
extern void *GetMovieReservedByIndex(void *, u8);
extern void func_8007CCB8(void);
/* PadRead declared in LIBETC.H */
extern void func_800259D4(s32, u32);
extern void func_80020E1C(void *);
extern void func_8001352C(s32);
extern void func_80020E80(void *);
/* DrawSync declared in LIBGPU.H */
extern s32 VSync(s32);
extern void func_80082C10(void);
extern void func_80013500(s32);

extern s32 D_800A5954;
extern u16 D_800A5958;
extern void *D_800A5960;
extern s32 D_800A5964;
extern s32 D_800A5968;
extern s32 D_800A597C;
extern s16 D_800A596C;
extern s16 D_800A596E;
extern s16 D_800A5970;

// sdata globals - these cause the non-match (GP-relative access issue)
extern u16 D_800A60BC;  // level count
extern u16 D_800A60BE;  // asset count
extern u8 D_800A60C0;   // total count
extern s32 D_800A6120;  // game data pointer copy

extern u8 D_8009DC40;
extern char *D_8009DE08[];
extern void *D_8009DDE0[];

void main(void) {
    void *gameState;
    void *levelData;
    s32 i;
    u32 padInput;
    s16 stateIndex;
    void (*stateFunc)(void *);
    s16 stateOffset;
    s32 vsyncCount;
    s16 baseOffset;

    __main();

    SsUtReverbOn(D_800A5954);
    ResetCallback();
    LoadGameAssetLocations();
    func_80013268(D_800A5954);

    D_800A5960 = &D_8009DC40;
    
    PadInit(0);
    InitGeom();
    SetDispMask(1);
    FntLoad(0x3C0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 0xC8, 0, 0x200));
    
    initPlayerState((u8 *)D_800A597C);
    func_8007CD34(&D_8009DC40, D_800A5964);

    levelData = (u8 *)D_800A5960 + 0x84;

    // Load level names
    for (i = 0; (i & 0xFF) < (GetLevelCount(levelData) & 0xFF); i++) {
        D_8009DE08[i & 0xFF] = getLevelName(levelData, i & 0xFF);
        D_800A60C0++;
        D_800A60BC++;
    }

    // Load additional assets
    for (i = 0; (i & 0xFF) < (GetAssetCount(levelData) & 0xFF); i++) {
        D_8009DDE0[D_800A60C0] = GetMovieReservedByIndex(levelData, i & 0xFF);
        D_800A60C0++;
        D_800A60BE++;
    }

    D_800A596C = 0x40;
    D_800A596E = 0x20;
    D_800A5970 = 0x80;
    D_800A6120 = D_800A5964;

    // Main game loop
    while (1) {
        func_8007CCB8();

        // Read controller input (only on first frame or when s0 == 0)
        padInput = PadRead(1);
        func_800259D4(D_800A5964, padInput & 0xFFFF);
        func_800259D4(D_800A5968, padInput >> 16);

        gameState = D_800A5960;
        stateIndex = *(s16 *)((u8 *)gameState + 2);
        
        if (stateIndex != 0) {
            if (stateIndex > 0) {
                // Lookup function from state table
                s32 *tablePtr = *(s32 *)((u8 *)gameState + *(s16 *)((u8 *)gameState + 4));
                stateOffset = *(s16 *)(tablePtr + stateIndex * 2 - 2);
                stateFunc = (void (*)(void *))(tablePtr + stateIndex * 2 - 1);
            } else {
                stateFunc = (void (*)(void *))*(s32 *)((u8 *)gameState + 4);
                stateOffset = 0;
            }

            baseOffset = *(s16 *)gameState;
            if ((stateIndex << 16) > 0) {
                baseOffset += stateOffset;
            }
            stateFunc((u8 *)gameState + baseOffset);
        }

        func_80020E1C(gameState);
        func_8001352C(D_800A5954);
        func_80020E80(gameState);
        DrawSync(0);

        // Secondary callback dispatch
        {
            void *cbStruct = *(void **)((u8 *)gameState + 0x18);
            s16 cbOffset = *(s16 *)((u8 *)cbStruct + 0x18);
            void (*cbFunc)(void *) = *(void (**)(void *))((u8 *)cbStruct + 0x1C);
            cbFunc((u8 *)gameState + cbOffset);
        }

        DrawSync(0);

        // VSync wait if flags set
        if (D_800A5958 & 6) {
            vsyncCount = VSync(-1) + 2;
            while (VSync(-1) < vsyncCount) {
                // wait
            }
        }

        func_80082C10();
        func_80013500(D_800A5954);
    }
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/main", main);
#endif

