/* =============================================================================
 * ProcessDebugMenuInput.c  --  PC-port debug level-select menu handler
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c ProcessDebugMenuInput (0x80082D..,
 * INCLUDE_ASM in src/main.c). Called once per frame at the end of the frame body.
 * The entire body is gated by the debug-menu-active flag (g_GameFlags & 0x80),
 * which is clear during normal play / the title screen, so this returns
 * immediately there. When active it prints the visible menu window, scrolls the
 * cursor with the d-pad (every 4th frame), and on Cross launches the selected
 * level or movie via SetSequenceIndexByMode + direct_level_load.
 *
 * The debug-branch-only symbols (g_MenuItemNames, g_szMovieNameEnd2,
 * SetSequenceIndexByMode, strcmp) are declared as externs; they are never
 * reached unless the debug flag is set. g_LevelCount is the D_800A60BC level
 * count mirror.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <string.h>

extern u16    g_LevelCount asm("D_800A60BC");
/* Export renames these; the real symbols (only touched in the never-taken debug
 * branch) are g_DebugMenuItemNames and g_DebugMenuSentinelStr -- weak-backed. */
extern char  *g_MenuItemNames asm("g_DebugMenuItemNames");
extern char   g_szMovieNameEnd2 asm("g_DebugMenuSentinelStr");

extern void FntPrint(void);
extern void SetSequenceIndexByMode(void *ctx, char mode, char index);

void ProcessDebugMenuInput(void) {
    if ((g_GameFlags & 0x80) != 0) {
        u16 i = g_DebugMenuScrollTop;
        u16 end = g_DebugMenuScrollTop + 0x14;
        if ((u16)g_TotalMenuItems < (u16)(g_DebugMenuScrollTop + 0x14)) {
            end = (u16)g_TotalMenuItems;
        }
        for (; i < end; i++) {
            FntPrint();
            FntPrint();
            FntPrint();
        }
        if ((g_pGameState->frame_counter & 3U) == 0) {
            if ((g_pCurrentInputState->buttons_held & PAD_UP) != 0) {
                if (g_DebugMenuCursor != 0) {
                    g_DebugMenuCursor = g_DebugMenuCursor - 1;
                }
                if (g_DebugMenuCursor < g_DebugMenuScrollTop) {
                    g_DebugMenuScrollTop = g_DebugMenuCursor;
                }
            }
            if ((g_pCurrentInputState->buttons_held & PAD_DOWN) != 0) {
                if ((int)(u32)g_DebugMenuCursor < (int)(g_TotalMenuItems - 1)) {
                    g_DebugMenuCursor = g_DebugMenuCursor + 1;
                }
                if ((int)(u32)g_DebugMenuScrollTop < (int)(g_DebugMenuCursor - 0x13)) {
                    g_DebugMenuScrollTop = g_DebugMenuCursor - 0x13;
                }
            }
        }
        if ((g_pCurrentInputState->buttons_pressed & 0x40) != 0) {
            u32  cursor = (u32)g_DebugMenuCursor;
            char cCursor = (char)g_DebugMenuCursor;
            u8   loadMode;
            g_GameFlags = g_GameFlags & 0xFF7F;
            if (cursor < (u32)(g_LevelCount + 10)) {
                if (cursor < 10) {
                    loadMode = cCursor + 1;
                } else {
                    loadMode = 'c';
                    SetSequenceIndexByMode(&g_pGameState->level_context, '\x04', cCursor - 10);
                }
            } else {
                if (strcmp((&g_MenuItemNames)[cursor], &g_szMovieNameEnd2) == 0) {
                    return;
                }
                loadMode = 'c';
                SetSequenceIndexByMode(&g_pGameState->level_context, '\x01',
                                       (cCursor - 10) - (char)g_LevelCount);
            }
            g_pGameState->direct_level_load = loadMode;
        }
    }
}
