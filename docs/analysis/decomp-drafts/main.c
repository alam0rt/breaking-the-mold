/* SHELVED DRAFT — main() decompilation, written from asm_annotate.py output.
 *
 * Status: compiles cleanly, gets within ~7 instructions of byte-match.
 *
 * REMAINING GAPS to fix for byte-match:
 *  1. Frame size 0x30 vs target 0x40 — cc1 doesn't allocate s2/s3 because
 *     local variables `i` (loop counter) and the D_8009DE08 base ptr
 *     don't trigger s-reg allocation. Target uses:
 *         s0 = inner u8 counter
 *         s1 = ctx (gs+0x84)
 *         s2 = outer loop counter
 *         s3 = D_8009DE08 base ptr
 *         s4 = argOff (mode dispatch)
 *         s5 = cb (mode dispatch)
 *     Tried `register x asm("$20")` pin for s4/s5 — works but shifts the
 *     others (s0=gs, s1=table, s4=counter, s5=ctx).
 *  2. Mode-dispatch register assignment: target uses callee-saved s4/s5 for
 *     argOff/cb (short lifetime, no jal between def/use). cc1 picks t-regs
 *     instead. Likely needs structural reshuffle of source (perhaps the
 *     original source had argOff/cb passed across a wider scope).
 *  3. With my +7 instructions, 96154.sdata.o shifts by 0x1C bytes, which
 *     corrupts the data section layout (g_pBlbHeapBase ends up at
 *     0x800A5970 instead of 0x800A5954, etc.).
 *
 * TENTATIVE-DEF DISCOVERY: the gp_rel quirk for half-word globals in .sdata
 * requires tentative defs (no `extern`, no initializer) — same pattern as
 * blbacc.c, gamecd.c, movie.c. Without them, cc1 emits lui/lo for every
 * access, costing 4 instructions per pair (and pushing the layout further).
 *
 * THE __main GOTCHA: gcc 2.7.2 auto-emits `jal __main` at the start of any
 * function named `main()`. This is the C-runtime ctor invocation. To make
 * the link work, `__main = 0x8008E6E0;` must be added to undefined_syms.txt
 * (NOT undefined_syms_auto.txt — splat regenerates that file on every
 * extract).
 *
 * Source below is the draft that compiles + builds + runs (but doesn't
 * byte-match). Use it as a starting point if revisiting this function. */

extern void SsUtReverbOn(void *base);
extern void ResetCallback(void);
extern void LoadGameAssetLocations(void);
extern void InitGraphicsSystem(void *base);
extern void PadInit(s32);
extern void InitGeom(void);
extern void SetDispMask(s32);
extern void FntLoad(s32, s32);
extern s32  FntOpen(s32, s32, s32, s32, s32, s32);
extern void SetDumpFnt(s32);
extern void InitGameState(GameState *gs, void *input);
extern u32  PadRead(s32);
extern void UpdateInputState(void *input, u16 raw);
extern void RenderEntities(GameState *gs);
extern s32  DrawSync(s32);
extern s32  VSync(s32);
extern void WaitForVBlankIfNeeded(void *base);
extern void FlushDebugFontAndEndFrame(void *base);
extern void TickCDStreamBuffer(void);
extern void ProcessDebugMenuInput(void);
extern u8   GetLevelCount(LevelDataContext *ctx);
extern char *getLevelName(LevelDataContext *ctx, u8 index);
extern u8   GetAssetCount(LevelDataContext *ctx);
extern u8  *GetMovieEntryByIndex(LevelDataContext *ctx, u8 index);
extern void initPlayerState(PlayerState *p);
extern void EntityTickLoop(GameState *gs);

extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");
extern void *D_800A5964 asm("D_800A5964");
extern void *D_800A5968 asm("D_800A5968");
extern s16 D_800A596C asm("D_800A596C");
extern s16 D_800A596E asm("D_800A596E");
extern s16 D_800A5970 asm("D_800A5970");
extern void *D_8009DE08[] asm("D_8009DE08");
/* Tentative defs unlock gp_rel via maspsx --use-comm-section (strong defs
 * live in 96154.sdata.s). */
u8  g_TotalMenuItems;
u16 D_800A60BC;
u16 D_800A60BE;
extern GameState g_GameStateBase;

void main(void) {
    u8 i;
    u32 padBits;
    GameState *gs;
    LevelDataContext *ctx;
    s32 modeId;
    s32 baseOff;
    void (*cb)(void *);
    s32 argOff;
    void *postCtx;
    s32 vsyncBase;

    /* gcc auto-emits `jal __main` here (resolved to 0x8008E6E0 via
     * undefined_syms.txt). It must be the first call in this function. */
    i = 0;
    SsUtReverbOn(g_pBlbHeapBase);
    ResetCallback();
    LoadGameAssetLocations();
    InitGraphicsSystem(g_pBlbHeapBase);
    g_pGameState = &g_GameStateBase;
    PadInit(0);
    InitGeom();
    SetDispMask(1);
    FntLoad(0x3C0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 0xC8, 0, 0x200));
    initPlayerState(PLAYER_STATE_DATA);
    InitGameState(&g_GameStateBase, D_800A5964);

    gs = g_pGameState;
    ctx = &gs->level_context;
    while ((u8)i < GetLevelCount(ctx)) {
        D_8009DE08[(u8)i] = getLevelName(ctx, (u8)i);
        g_TotalMenuItems++;
        D_800A60BC++;
        i++;
    }
    i = 0;
    while ((u8)i < GetAssetCount(ctx)) {
        g_DebugMenuItemNames[g_TotalMenuItems] = GetMovieEntryByIndex(ctx, (u8)i);
        g_TotalMenuItems++;
        D_800A60BE++;
        i++;
    }
    D_800A596C = 0x40;
    D_800A596E = 0x20;
    D_800A5970 = 0x80;
    g_pCurrentInputState = D_800A5964;

    padBits = 0;
    do {
        TickCDStreamBuffer();
        if ((u8)padBits == 0) {
            padBits = PadRead(1);
            UpdateInputState(D_800A5964, (u16)padBits);
            UpdateInputState(D_800A5968, padBits >> 16);
        }
        modeId = ((s16 *)g_pGameState)[1];
        if (modeId != 0) {
            argOff = 0;
            if (modeId > 0) {
                s16 tblOff = ((s16 *)g_pGameState)[2];
                s32 *table = *(s32 **)((char *)g_pGameState + tblOff);
                argOff = table[(modeId - 1) * 2];
                cb = (void (*)(void *))table[(modeId - 1) * 2 + 1];
            } else {
                cb = (void (*)(void *))((s32 *)g_pGameState)[1];
            }
            baseOff = ((s16 *)g_pGameState)[0];
            if (modeId > 0) {
                baseOff += (s16)argOff;
            }
            cb((char *)g_pGameState + baseOff);
        }
        EntityTickLoop(g_pGameState);
        WaitForVBlankIfNeeded(g_pBlbHeapBase);
        RenderEntities(g_pGameState);
        DrawSync(0);
        postCtx = g_pGameState->postRenderCallbackContext;
        (*(void (**)(void *))((char *)postCtx + 0x1C))(
            (char *)g_pGameState + *(s16 *)((char *)postCtx + 0x18));
        DrawSync(0);
        if (g_GameFlags & 0x6) {
            vsyncBase = VSync(-1) + 2;
            while ((u32)VSync(-1) < (u32)vsyncBase) {
            }
        }
        ProcessDebugMenuInput();
        padBits = 0;
        FlushDebugFontAndEndFrame(g_pBlbHeapBase);
    } while (1);
}
