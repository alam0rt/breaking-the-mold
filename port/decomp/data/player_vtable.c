/* =============================================================================
 * player_vtable.c  --  PC-port strong backing for g_PlayerCallbackTable
 * =============================================================================
 * Transcribed from asm/data/clayball.rodata.s @ 0x80011804 (0x80 bytes; the
 * first 16 words are populated, the tail is the zero pad splat carved).
 * Two stacked 8-slot {arg, fn} pair tables: the standard entity vtable
 * (destroy / tick=UpdateEntityRender / post-render=UploadEntityTextureIfDirty)
 * plus the extended player slots (Finn-shared callbacks).
 * ========================================================================== */

extern char EntityDestructor_WithSPUVoiceStop[];
extern char UpdateEntityRender[];
extern char UploadEntityTextureIfDirty[];
extern char FinnSubentityDestroyCallback_Simple[];
extern char FinnNoopCallback_8006ED4C[];
extern char FinnNoopCallback_8006ED44[];

void *g_PlayerCallbackTable_vt[32] asm("g_PlayerCallbackTable") = {
    0, 0,
    0, (void *)EntityDestructor_WithSPUVoiceStop,
    0, (void *)UpdateEntityRender,
    0, (void *)UploadEntityTextureIfDirty,
    0, 0,
    0, (void *)FinnSubentityDestroyCallback_Simple,
    0, (void *)FinnNoopCallback_8006ED4C,
    0, (void *)FinnNoopCallback_8006ED44,
    /* remaining 16 words are zero in ROM */
};
