#include "common.h"

/*
 * main() - Main game entry point and game loop
 * 
 * Initialization sequence:
 *   1. __main()              - C runtime init (static constructors)
 *   2. SsUtReverbOn()        - Enable sound reverb
 *   3. ResetCallback()       - Reset system callbacks
 *   4. LoadGameAssetLocations() - Find game assets on CD
 *   5. func_80013268()       - Unknown init
 *   6. D_800A5960 = &D_8009DC40 - Set global game state pointer
 *   7. PadInit(0)            - Initialize controller
 *   8. InitGeom()            - Initialize GTE geometry
 *   9. SetDispMask(1)        - Enable display
 *  10. FntLoad(960, 256)     - Load debug font to VRAM
 *  11. FntOpen(16, 32, 288, 200, 0, 512) - Open font stream
 *  12. SetDumpFnt()          - Set debug font output
 *  13. initPlayerState()     - Initialize player state
 *  14. func_8007CD34()       - Unknown setup with game data
 * 
 * Level loading loop:
 *   - Iterates through levels, calling getLevelName() for each
 *   - Stores level name pointers in D_8009DE08 array
 *   - Increments D_800A60C0 (total count) and D_800A60BC (level count)
 * 
 * Secondary asset loop:
 *   - Loads additional assets via func_8007ACDC/func_8007ACF0
 *   - Stores in D_8009DDE0 array ("sub_01" strings?)
 *   - Increments D_800A60C0 and D_800A60BE
 * 
 * Sets display constants:
 *   - D_800A596C = 0x40 (64)
 *   - D_800A596E = 0x20 (32)
 *   - D_800A5970 = 0x80 (128)
 * 
 * Main game loop (infinite):
 *   1. func_8007CCB8()       - Frame start / timing sync
 *   2. PadRead(1)            - Read controller input
 *   3. func_800259D4()       - Process input for both pads
 *   4. State machine dispatch:
 *      - Reads state index from gameState[1] (offset 0x02)
 *      - If > 0: looks up function pointer from table at gameState[2]
 *      - If < 0: uses gameState[2] directly as function pointer
 *      - Calls the function with game state + offset
 *   5. func_80020E1C()       - Process game state
 *   6. func_8001352C()       - Unknown (with PTR_DAT_800A5954)
 *   7. func_80020E80()       - More game state processing
 *   8. DrawSync(0)           - Wait for GPU drawing to complete
 *   9. Callback dispatch from game state structure
 *  10. DrawSync(0)           - Wait again
 *  11. VSync handling:
 *      - If D_800A5958 & 6: wait for 2 VSyncs
 *  12. func_80082C10()       - End of frame processing
 *  13. func_80013500()       - Frame cleanup
 *  14. Loop back to step 1
 * 
 * Key globals:
 *   - D_800A5954: Pointer used with sound/callback functions
 *   - D_800A5958: Flags (bit 1-2 control VSync wait)
 *   - D_800A5960: Main game state structure pointer -> D_8009DC40
 *   - D_800A5964: Game data pointer (levels?)
 *   - D_800A5968: Secondary game data pointer
 *   - D_800A597C: Player state pointer
 * 
 * NOTE: This function cannot be decompiled to matching C code because it
 * accesses sdata variables (D_800A60BC, D_800A60BE, D_800A60C0, D_800A6120)
 * using GP-relative addressing. GCC 2.7.2 only generates GP-relative code
 * for locally-defined variables, not externs.
 */

#if 0
// Non-matching decompiled code for reference
// Accesses to sdata globals will generate incorrect (non-GP-relative) code

extern void __main(void);
extern void SsUtReverbOn(s32);
extern void ResetCallback(void);
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
extern u8 func_8007A9B0(void *);
extern char *getLevelName(void *, u8);
extern u8 func_8007ACDC(void *);
extern void *func_8007ACF0(void *, u8);
extern void func_8007CCB8(void);
extern u32 PadRead(s32);
extern void func_800259D4(s32, u32);
extern void func_80020E1C(void *);
extern void func_8001352C(s32);
extern void func_80020E80(void *);
extern void DrawSync(s32);
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
    for (i = 0; (i & 0xFF) < (func_8007A9B0(levelData) & 0xFF); i++) {
        D_8009DE08[i & 0xFF] = getLevelName(levelData, i & 0xFF);
        D_800A60C0++;
        D_800A60BC++;
    }

    // Load additional assets
    for (i = 0; (i & 0xFF) < (func_8007ACDC(levelData) & 0xFF); i++) {
        D_8009DDE0[D_800A60C0] = func_8007ACF0(levelData, i & 0xFF);
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

            s16 baseOffset = *(s16 *)gameState;
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

