#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* =============================================================================
 * Cross-file function prototypes
 *
 * Declarations for engine functions that are called from many translation
 * units. Previously each .c file carried its own local `extern` copy (e.g.
 * FreeFromHeap was redeclared in 18 files); this header is the single source
 * of truth so the signatures can't drift.
 *
 * CLEAN-ROOM: names and types are inferred working labels. Where a function is
 * also *defined* in src/ (DestroyEntityAndFreeMemory in entity.c, StopSPUVoice
 * in sound.c) the prototype here matches that definition exactly. Argument
 * types are all register-width (pointers / s32), so substituting these
 * prototypes for the old per-file externs is codegen-identical - the one
 * exception is EntityApplyMovementCallbacks, whose s16 params are preserved
 * verbatim because narrowing the param type would change caller codegen.
 * ============================================================================= */

#include "common.h"
#include "Game/entity.h"

/* ---- BLB heap -------------------------------------------------------------- */
extern void *AllocateFromHeap(void *heap, s32 align, s32 size, s32 flags);
extern void  FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);

/* ---- Entity lifecycle / lists ---------------------------------------------- */
extern void DestroyEntityAndFreeMemory(SpriteEntity *entity, s32 flags);
extern void EntityProcessCallbackQueue(Entity *entity);
extern void EntityUpdateCallback(Entity *entity);
extern void FreeEntityLists(void *entity);
extern void RemoveFromTickList(void *entity, void *child);
extern u8   EntityApplyMovementCallbacks(void *entity, s16 x, s16 y);
extern void AddEntityToSortedRenderList(void *list, void *entity);

/* ---- Render primitives ----------------------------------------------------- */
extern void AddPrim(void *ot, void *prim);

/* ---- Audio ----------------------------------------------------------------- */
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void StopSPUVoice(s32 voice);

/* ---- HUD ------------------------------------------------------------------- */
extern void HidePauseMenuHUD(s32 handle);
/* NOTE: GetLevelFlags is intentionally NOT centralized. It is defined in
 * blbacc.c as u16 GetLevelFlags(LevelDataContext*), but main.c/lvlload.c/
 * entinit.c call it through an unprototyped `extern s32 GetLevelFlags();` and
 * rely on that K&R form (arg + return-width handling). Forcing one prototype
 * changes codegen, so each TU keeps its own local declaration. */

/* ---- libc ------------------------------------------------------------------ */
extern s32 rand(void);

#endif /* FUNCTIONS_H */
