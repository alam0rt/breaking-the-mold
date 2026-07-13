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
#include "Game/asset_ids.h"
#include "Game/entity.h"
#include "Game/entity_events.h"

/* ---- BLB heap -------------------------------------------------------------- */
extern u8   *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void  FreeFromHeap(u8 *heap, u8 *ptr, s32 mode, s32 flags);

/* ---- Entity lifecycle / lists ---------------------------------------------- */
extern void DestroyEntityAndFreeMemory(SpriteEntity *entity, s32 flags);
extern void EntityProcessCallbackQueue(Entity *entity);
extern void EntityUpdateCallback(Entity *entity);
extern void FreeEntityLists(Entity *entity);
extern s32  RemoveFromTickList(u8 *gs, void *child); /* returns 1 if unlinked */
extern u8   EntityApplyMovementCallbacks(Entity *entity, s16 x, s16 y);
extern void AddEntityToSortedRenderList(u8 *list, Entity *entity);

/* ---- Render primitives ----------------------------------------------------- */
extern void AddPrim(u8 *ot, u8 *prim);

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
