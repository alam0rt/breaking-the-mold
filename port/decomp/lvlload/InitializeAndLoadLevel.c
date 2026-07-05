/* =============================================================================
 * InitializeAndLoadLevel.c  --  PC-port level asset loader (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/lvlload/InitializeAndLoadLevel.s
 * (460 lines, @0x8007D1D0), from the m2c decompilation (jump table jtbl_80012140
 * supplied so m2c could resolve the playback-sequence switch). This is the level
 * asset/tile/sprite/entity-spawn entry the boot's InitGameState calls with
 * mode=0x63.
 *
 * Structure:
 *   1. preamble: clear heap latch bytes; flush + vblank; drop from lists; SPU pop.
 *   2. if mode==0x63: run the movie/loading-screen PLAYBACK SEQUENCE loop (switch
 *      on AdvancePlaybackSequence()->0-5), then LevelDataParser + audio upload.
 *   3. tail: LoadAssetContainer -> asset audio -> carve the level sub-heap
 *      (InitHeapConfig/InitHeapFreeList/AllocateFromHeap [our verified heap]) ->
 *      InitVRAMSlotTable -> reset the game-mode dispatch + entity lists ->
 *      LoadTileDataToVRAM -> LoadAssetContainer -> free scratch -> tile/color/
 *      entity/layer init. Returns the loading-screen-active flag.
 *
 * Raw byte offsets are used throughout (byte-accurate to the asm) to avoid
 * coupling to GameState struct-header naming. arg0 = GameState*, mode = 0x63 on
 * the boot path. Still-asm callees (movie/loading funcs, LevelDataParser,
 * LoadAssetContainer, LoadTileDataToVRAM, the tile/entity/layer loaders) resolve
 * to weak stubs until converted -- the first reached is the next task.
 *
 * PC MEMORY-MODEL NOTE: the sub-heap InitHeapConfig call uses the PSX RAM-top
 * constant 0x801FC000; InitHeapConfig clamps the resulting (garbage-on-PC) size
 * to 0xFFFF0, and the staging buffer (D_800AE3E0, sized 4 MB in render_core.c)
 * holds it. See docs/plans/pc-port.md CP-2.2.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

/* Absolute-address globals (asm aliases so the stub generator backs them weakly). */
extern u8  *RESPAWN_PLAYER_STATE asm("D_800A597C");       /* -> player state    */
extern s32  D_800A60A4 asm("D_800A60A4");                 /* asset-load state    */
extern u8   MOVIE_NAME_608C[] asm("D_800A608C");          /* strcmp targets      */
extern u8   MOVIE_NAME_6094[] asm("D_800A6094");
extern u8   MOVIE_NAME_609C[] asm("D_800A609C");
extern void *GameModeCallback;
extern void *EntityDestructCallback;

/* Level-load callees (mix of matched C and still-asm). */
extern void FlushDebugFontAndEndFrame(void *heap);
extern void WaitForVBlankIfNeeded(void *heap);
extern void RemoveFromUpdateQueue(void *gs);
extern void RemoveFromZOrderList(void *gs);
extern void ClearEntityDefList(void *gs);
extern void PopSPUUploadBlock(void);
extern s8   AdvancePlaybackSequence(void *ctx);
extern void SeekToLevelInSequence(void *ctx, u32 a1, s32 a2, s32 a3);
extern s16  GetCurrentSectorOffset(void *ctx);
extern s16  GetCurrentSectorCount(void *ctx);
extern s8   GetLoadingScreenMinDisplayTime(void *ctx);
extern s8   GetLoadingScreenMaxDisplayTime(void *ctx);
extern s8   DisplayLoadingScreen(s32 off, s32 cnt, void *buf, s32 mn, s32 mx);
extern void InitializePlayerState(void *ps);
extern s32  GetCurrentMovieReserved(void *ctx);
extern s32  GetCurrentMovieFilename(void *ctx);
extern s16  GetMovieFrameField00(void *ctx);
extern s16  GetMovieSectorCount(void *ctx);
extern s8   PlayMovieFromCD(s32 fn, s32 f0, s32 sc, void *buf);
extern s8   PlayMovieFromBLBSectors(s32 off, s32 cnt, void *buf);
extern s32  GetCurrentModeReservedData(void *ctx);
extern void LevelDataParser(void *ctx, void *levelData);
extern void *GetAsset601Ptr(void *ctx);
extern void *GetAsset602Ptr(void *ctx);
extern s32  GetAsset601Size(void *ctx);
extern void UploadAudioToSPU(void *a601, void *a602, s32 sz);
extern s8   GetCurrentLevelAssetIndex(void *ctx);
extern void LoadAssetContainer(void *ctx, s32 mode, s32 flags);
extern s32  GetPrimaryBufferSize(void *ctx);
extern s32  GetCurrentTertiaryDataSize(void *ctx);
extern void InitHeapConfig(void *heap, u8 *ptr, u32 size);
extern void InitHeapFreeList(void *heap);
extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void ClearHeapBlocks(void *heap, s32 fill);
extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);
extern s8   GetAsset101Entry(void *ctx, s32 which);
extern void InitVRAMSlotTable(void *heap, int a1, int a2);
extern void LoadTileDataToVRAM(void *gs);
extern void InitPlayerSpawnPosition(void *gs);
extern void LoadBGColorFromTileHeader(void *gs);
extern void LoadSecondaryColorFromTileHeader(void *gs);
extern void AddPreInitEntitiesToList(void *gs);
extern void InitTileAttributeState(void *gs);
extern void LoadEntitiesFromAsset501(void *gs);
extern void InitAnimatedTileEntities(void *gs);
extern void InitLayersAndTileState(void *gs);

extern s32  strcmp(const char *a, const char *b);

s32 InitializeAndLoadLevel(void *arg0, u8 mode) {
    u8 *gs = (u8 *)arg0;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *ctx = gs + 0x84;
    s32 s6 = mode & 0xFF;
    s32 s7 = 0;                 /* return: loading-screen-active flag */

    heap[0x1D] = 0; heap[0x1E] = 0; heap[0x1F] = 0;
    heap[0x505D] = 0; heap[0x505E] = 0; heap[0x505F] = 0;
    FlushDebugFontAndEndFrame(heap);
    WaitForVBlankIfNeeded(heap);
    RemoveFromUpdateQueue(arg0);
    RemoveFromZOrderList(arg0);
    ClearEntityDefList(arg0);
    PopSPUUploadBlock();

    if (s6 == 0x63) {
        s32 seq;
        s6 = 1;
        PopSPUUploadBlock();
        for (;;) {
            seq = AdvancePlaybackSequence(ctx) & 0xFF;
            switch (seq) {
            case 0:
                SeekToLevelInSequence(ctx, 0, 1, 0);
                break;
            case 4:
            case 5: {
                s32 off, cnt, mn;
                FlushDebugFontAndEndFrame(heap);
                WaitForVBlankIfNeeded(heap);
                off = GetCurrentSectorOffset(ctx) & 0xFFFF;
                cnt = GetCurrentSectorCount(ctx) & 0xFFFF;
                mn  = GetLoadingScreenMinDisplayTime(ctx) & 0xFF;
                s7 = (DisplayLoadingScreen(off, cnt, *(void **)(gs + 0x3C), mn,
                        GetLoadingScreenMaxDisplayTime(ctx) & 0xFF) & 0xFF) == 0;
                if (s7 & 0xFF) {
                    s32 s5;
                    SeekToLevelInSequence(ctx, 0, 1, 0);
                    s5 = AdvancePlaybackSequence(ctx);
                    FlushDebugFontAndEndFrame(heap);
                    WaitForVBlankIfNeeded(heap);
                    off = GetCurrentSectorOffset(ctx) & 0xFFFF;
                    cnt = GetCurrentSectorCount(ctx) & 0xFFFF;
                    mn  = GetLoadingScreenMinDisplayTime(ctx) & 0xFF;
                    DisplayLoadingScreen(off, cnt, *(void **)(gs + 0x3C), mn,
                        GetLoadingScreenMaxDisplayTime(ctx) & 0xFF);
                    SeekToLevelInSequence(ctx, RESPAWN_PLAYER_STATE[0], 0, 0);
                    s6 = RESPAWN_PLAYER_STATE[1];
                    InitializePlayerState(RESPAWN_PLAYER_STATE);
                    seq = s5 & 0xFF;
                }
                break;
            }
            case 1:
                if (strcmp((char *)GetCurrentMovieReserved(ctx), (char *)MOVIE_NAME_608C) != 0
                    || (u8)RESPAWN_PLAYER_STATE[0x1B] >= 0x30) {
                    s32 fn = GetCurrentMovieFilename(ctx);
                    s32 f0 = GetMovieFrameField00(ctx);
                    if (!(PlayMovieFromCD(fn, f0, GetMovieSectorCount(ctx),
                                          *(void **)(gs + 0x3C)) & 0xFF)) {
                        if (strcmp((char *)GetCurrentModeReservedData(ctx),
                                   (char *)MOVIE_NAME_6094) == 0) {
                            AdvancePlaybackSequence(ctx);
                            AdvancePlaybackSequence(ctx);
                            AdvancePlaybackSequence(ctx);
                            seq = 3;   /* -> loop exit */
                        }
                    }
                }
                break;
            case 2: {
                s32 off = GetCurrentSectorOffset(ctx) & 0xFFFF;
                if (!(PlayMovieFromBLBSectors(off, GetCurrentSectorCount(ctx) & 0xFFFF,
                                              *(void **)(gs + 0x3C)) & 0xFF)) {
                    if (strcmp((char *)GetCurrentModeReservedData(ctx),
                               (char *)MOVIE_NAME_609C) == 0) {
                        AdvancePlaybackSequence(ctx);
                        AdvancePlaybackSequence(ctx);
                        AdvancePlaybackSequence(ctx);
                        seq = 3;   /* -> loop exit */
                    }
                }
                break;
            }
            default:
                break;   /* case 3 (terminal): fall through to exit check */
            }
            if (seq == 3 || seq == 6) {
                break;
            }
        }
        LevelDataParser(ctx, *(void **)(gs + 0x3C));
        {
            void *a601 = GetAsset601Ptr(ctx);
            void *a602 = GetAsset602Ptr(ctx);
            UploadAudioToSPU(a601, a602, GetAsset601Size(ctx));
        }
    }

    /* ---- tail (.L8007D56C): the real level asset -> heap -> VRAM load -------- */
    {
        u8 *arg = ctx;
        if (!(GetCurrentLevelAssetIndex(ctx) & 0xFF) && s6 == 1) {
            s6 = 5;
            if (D_800A60A4 == 4) {
                D_800A60A4 = 0;
            } else {
                if (D_800A60A4 == 0) {
                    s6 = 1;
                } else if (D_800A60A4 == 2) {
                    s6 = 6;
                } else {
                    s6 = 1;
                }
                D_800A60A4 += 1;
            }
        }
        LoadAssetContainer(arg, s6 & 0xFF, 1);

        {
            void *a601 = GetAsset601Ptr(ctx);
            void *a602 = GetAsset602Ptr(ctx);
            UploadAudioToSPU(a601, a602, GetAsset601Size(ctx));
        }

        {
            s32 primBuf = GetPrimaryBufferSize(ctx);
            s32 tertiary = (GetCurrentTertiaryDataSize(ctx) + 0xF) & ~0xF;
            s32 scratchSize;
            u8 *scratch = NULL;
            u8 *heapBase = (u8 *)g_pBlbHeapBase;
            u8 *subheapStart;

            if (tertiary == 0) {
                tertiary = primBuf;
            }
            scratchSize = primBuf - tertiary;
            subheapStart = *(u8 **)(gs + 0x3C) + tertiary;
            InitHeapConfig(heapBase, subheapStart,
                           (u32)(0x801FC000 - (s32)(uintptr_t)subheapStart));
            InitHeapFreeList(heapBase);
            if (scratchSize != 0) {
                scratch = AllocateFromHeap(heapBase, 1, scratchSize, 1);
            }
            ClearHeapBlocks(heapBase, 0xEEEEEEEE);

            {
                s32 a0 = GetAsset101Entry(ctx, 0);
                s32 a1 = GetAsset101Entry(ctx, 1);
                if (a0 == 0 && a1 == 0) {
                    a1 = 2;
                }
                InitVRAMSlotTable(heapBase, a0 & 0xFF, a1 & 0xFF);
            }

            *(s16 *)(gs + 0x104) = 0;
            *(s32 *)(gs + 0x108) = 0;

            /* game-mode dispatch: marker 0 / table_offset -1 / callback. */
            *(s32 *)(gs + 0x0) = (s32)0xFFFF0000;
            *(void **)(gs + 0x4) = &GameModeCallback;
            *(s32 *)(gs + 0x8) = (s32)0xFFFF0000;
            *(void **)(gs + 0xC) = &EntityDestructCallback;

            /* entity/render-list + scratch field reset (byte-accurate to asm). */
            *(s32 *)(gs + 0x1C) = 0; *(s32 *)(gs + 0x20) = 0;
            *(s32 *)(gs + 0x24) = 0; *(s32 *)(gs + 0x28) = 0;
            *(s32 *)(gs + 0x2C) = 0; *(s32 *)(gs + 0x30) = 0;
            *(s32 *)(gs + 0x4C) = 0; *(s32 *)(gs + 0x50) = 0;
            gs[0x58] = 0; gs[0x59] = 0; gs[0x5A] = 0; gs[0x5B] = 0; gs[0x60] = 0;
            *(s16 *)(gs + 0x5C) = 0; *(s16 *)(gs + 0x5E) = 0;
            gs[0x61] = 0; gs[0x62] = 0; gs[0x11A] = 0;
            *(s16 *)(gs + 0x54) = 0; *(s16 *)(gs + 0x56) = 0;
            *(s32 *)(gs + 0x34) = 0; *(s32 *)(gs + 0x38) = 0;
            *(s32 *)(gs + 0x74) = 0; *(s16 *)(gs + 0x78) = 0;
            *(s32 *)(gs + 0x7C) = 0; *(s16 *)(gs + 0x80) = 0;
            *(s16 *)(gs + 0x44) = 0; *(s16 *)(gs + 0x46) = 0;
            *(s32 *)(gs + 0x10C) = 0; *(s32 *)(gs + 0x110) = 0;
            gs[0x114] = 0; gs[0x63] = 0;
            *(s16 *)(gs + 0x120) = 0; *(s16 *)(gs + 0x122) = 0;
            gs[0x12A] = 0x28;

            LoadTileDataToVRAM(arg0);
            LoadAssetContainer(ctx, s6 & 0xFF, 0);
            if (scratch != NULL) {
                FreeFromHeap(heapBase, scratch, scratchSize, 1);
            }
            ClearHeapBlocks(heapBase, 0xEEEEEEEE);
            /* minFree = freeCount (base+0xA64E = base+0xA64C). */
            *(u16 *)(heapBase + 0xA64E) = *(u16 *)(heapBase + 0xA64C);
        }
    }

    InitPlayerSpawnPosition(arg0);
    LoadBGColorFromTileHeader(arg0);
    LoadSecondaryColorFromTileHeader(arg0);
    AddPreInitEntitiesToList(arg0);
    InitTileAttributeState(arg0);
    LoadEntitiesFromAsset501(arg0);
    InitAnimatedTileEntities(arg0);
    InitLayersAndTileState(arg0);
    return s7;
}
