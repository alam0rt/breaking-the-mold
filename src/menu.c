#include "common.h"
#include "Game/entity.h"

extern void EntityProcessCallbackQueue(Entity *entity);
extern void EntityUpdateCallback(Entity *entity);
extern void SetEntitySpriteId(Entity *entity, u32 spriteId, s32 flags);
extern void SetAnimationFrameIndex(void *animEntity, u32 value);
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void ApplyAudioSettings(void *audioCtx);
extern s32 rand(void);
extern void InitRunnLevelEntity(Entity *entity);
extern void *InitEntitySprite(void *entity, u32 spriteId, s32 z,
                              s16 x, s16 y, s32 flags);
extern void *InitEntityWithSprite(void *entity, void *spriteDef, s32 z,
                                  s16 x, s16 y);
extern void AttachCursorToButton(Entity *button);
extern void *AllocateFromHeap(void *heap, s32 size, s32 align, s32 flags);
extern void AddEntityToSortedRenderList(void *list, void *entity);
extern void *g_pBlbHeapBase;
extern void *g_pGameState;
extern u8 D_80012034[]; /* menu-button vtable, +0x18 install slot */
extern u8 D_8001208C[]; /* menu-button-highlight vtable */
extern u8 D_80011FDC[]; /* password-button vtable (post-cursor override) */
extern u8 D_8009CBE8[]; /* menu-button-highlight sprite table */
extern s32 GetWorldPositionX(Entity *entity, s16 localX);
extern s32 GetWorldPositionY(Entity *entity, s16 localY);

/* Forward decls for in-TU helpers used by Setup*Animation before their
 * own definitions appear below. */
void MenuSetEntityIdle2(Entity *entity);
void TimerEntityTick(Entity *entity);
void SetupMenuIdleAnimation(Entity *entity);

/* Local 8-byte (markerLo, markerHi, fn) FSM callback-install slot used
 * by the menu set-idle helpers. Wrapped in a padded 2-element array to
 * reproduce the 4-byte stack hole + 8 bytes of trailing scratch in the
 * original codegen (cc1 Quirk 3 in docs/compiler-quirks.md). */
typedef struct {
    s16  markerLo;
    s16  markerHi;
    void (*fn)(Entity *);
} MenuCallbackSlot;

typedef struct {
    s32 pad;
    MenuCallbackSlot s[2];
} PaddedSlotPair;

/* Variant with trailing pad: used in activate-button helpers where 3
 * callee-saved regs are spilled. Sandwiches the slot between two pads so
 * cc1 puts the slot at sp+0x14 with frame 0x30, same as PaddedSlotPair
 * in the simpler set-idle helpers. */
typedef struct {
    s32 pad;
    MenuCallbackSlot s;
    s32 pad2;
} TripadSlot;

/* Allocates+inits the menu cursor SpriteEntity via InitEntityWithSprite
 * using the D_8009CBDC sprite table at z-order 0x7D0 (2000) and the
 * caller-supplied (x,y). Installs vtable D_800120AC, wires
 * GetWorldPositionX/Y as identity move-callbacks at +0x24/+0x2C (FSM
 * marker 0xFFFF0000 = direct call), then EntitySetState to the FSM
 * slot at D_800A6050/D_800A6054 to enter the cursor's idle tick state.
 *
 * SHELVED: gp-rel-extern blocker. Original loads D_800A6050/D_800A6054 via
 * single-instruction `lw $aN, %gp_rel(D_800A6050)($gp)`. Our cc1+maspsx
 * cannot emit gp_rel addressing for `extern u32 D_800A6050` (the symbol is
 * owned by undefined_syms.txt, not defined in menu.o's .sdata). cc1 emits
 * the bare symbol form and maspsx leaves it for GNU as, which resolves to
 * 2-instruction `lui $aN, %hi(...); lw $aN, %lo(...)($aN)`. Net: function
 * grows by 8 bytes (2 gp_rel sites × 4 bytes each), shifting every byte
 * after it including all .data — symbol D_8009CBDC slides to 0x8009CBE4,
 * causing 29k+ vtable byte diffs cascading through libgpu etc.
 * See memories/repo/gp-rel-extern-blocker.md. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuCursorEntity);

/* Generic tick callback that drives a countdown timer at entity+0x100:
 * if nonzero, decrements it and on reaching 0 fires
 * EntityProcessCallbackQueue to consume the queued next-state callback
 * (+0x98/+0x9C). Always falls through to EntityUpdateCallback for the
 * regular per-frame sprite/animation/render housekeeping. */
void TimerEntityTick(Entity *entity) {
    u16 *timer = (u16 *)((u8 *)entity + 0x100);
    u16 v = *timer;
    if (v != 0) {
        v -= 1;
        *timer = v;
        if (v == 0) {
            EntityProcessCallbackQueue(entity);
        }
    }
    EntityUpdateCallback(entity);
}

/* Minimal event-callback for the Klaymen/runner-style idle helper
 * entities. Returns 0 for every event id except 2, which dispatches
 * EntityProcessCallbackQueue to fire the queued state transition
 * (typical "animation finished" hand-off). */
s32 MenuEntityCallback(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

/* Sets up the menu-character "idle then loop" state. Seeds entity+0x100
 * with a randomised hold timer (0xF0 + rand()&0x7F frames), installs
 * TimerEntityTick at the tick slot (+0x00/+0x04) and clears the event
 * pair (+0x08/+0x0C), switches sprite id to 0x0005C699 via
 * SetEntitySpriteId, and queues InitRunnLevelEntity at +0x98/+0x9C so
 * the run animation kicks in when the timer expires.
 *
 * SHELVED: 4-instruction scheduling diff. cc1 in the original hoists
 * `li s1, -1` (the marker constant) BEFORE the timer write (between
 * the rand-bitmask chain and the `sh v0, 0x100(s0)`); modern cc1
 * places it after. Tried statement reorderings; either shifts the diff
 * around or grows it. Close enough that decomp-permuter would likely
 * close it. */
INCLUDE_ASM("asm/nonmatchings/menu", SetupMenuIdleAnimation);

/* Transition handler that swaps the menu character from idle to its
 * active running animation. Installs EntityUpdateCallback at the tick
 * slot and MenuEntityCallback as the event handler, switches sprite to
 * 0x0305A4F5, and queues SetupMenuIdleAnimation at +0x98/+0x9C so the
 * cycle re-enters idle once the run animation completes.
 *
 * SHELVED: 4-instruction scheduling diff. TripadSlot pins the 0x30
 * frame correctly but cc1 in the original places `sw $ra` between the
 * `sw $s0` save and `li s0, -1`, then schedules markerLo store before
 * markerHi. Modern cc1 does `sw s0; li s0, -1` immediately together
 * and stores markerHi first. Same diff class as SetupMenuIdleAnimation
 * (4-instr reorder). Permuter candidate. */
INCLUDE_ASM("asm/nonmatchings/menu", InitRunnLevelEntity);

/* Allocates+inits a "button highlight" SpriteEntity from the D_8009CBE8
 * sprite table at z-order 0x7D0 (2000), with sign-extended s16 (x,y)
 * supplied by the caller. Installs vtable D_8001208C and the
 * GetWorldPositionX/Y identity move-callbacks at +0x24/+0x2C so the
 * sprite tracks its own world position. Returns the new entity.
 *
 * SHELVED: register allocation diff. cc1 in the original allocates
 * $v1 for the GetWorldPosition function pointer and reuses $v0 for the
 * -1 marker (after the vtable store frees $v0). Modern cc1 picks $v0
 * for fn ptr and $v1 for marker — same body, opposite regs. Tried
 * statement re-ordering (markerFirst, fnFirst, interleaved); all swap
 * which regs get the swap. Permuter territory. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuButtonEntity);

/* Event-handler installed on the button-highlight child by
 * MenuActivate*Button helpers. Same body as MenuEntityCallback: only
 * event id 2 triggers EntityProcessCallbackQueue, which consumes the
 * queued MenuSetEntityIdle2 transition that returns the highlight to
 * its resting sprite after the activation flash. */
s32 MenuButtonCallback(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

/* Primes a freshly-built button highlight with the press-flash sequence.
 * Installs MenuButtonCallback at +0x08/+0x0C, sets the current sprite
 * id to 0x63848E59 (highlighted), and queues MenuSetEntityIdle2 at
 * +0x98/+0x9C so the next callback-queue dispatch settles back to the
 * idle sprite.
 *
 * SHELVED: Quirk-5 lui-hoist scheduling diff. Same family as
 * MenuActivateButton; TripadSlot pins the 0x30 frame correctly but
 * cc1 in the original schedules `li s1, -1` immediately after the
 * `sw s1, ...` save (instr 4) while modern cc1 places it later. */
INCLUDE_ASM("asm/nonmatchings/menu", SetupMenuButtonAnimation);

/* Tail-call helper: clears the event-callback pair at +0x08/+0x0C and
 * switches the sprite id to 0x39900619 (the primary "idle"
 * button-highlight sprite) via SetEntitySpriteId. */
void MenuSetEntityIdle(Entity *entity) {
    PaddedSlotPair u;
    u.s[0].markerLo = 0;
    u.s[0].markerHi = 0;
    u.s[0].fn = NULL;
    *(MenuCallbackSlot *)((u8 *)entity + 8) = u.s[0];
    SetEntitySpriteId(entity, 0x39900619, 1);
}

/* Variant of MenuSetEntityIdle that switches to the alternate idle
 * sprite 0x33808E1B; otherwise identical (clears event pair, calls
 * SetEntitySpriteId). Used as the post-flash resting state queued by
 * MenuActivateButton-family helpers. */
void MenuSetEntityIdle2(Entity *entity) {
    PaddedSlotPair u;
    u.s[0].markerLo = 0;
    u.s[0].markerHi = 0;
    u.s[0].fn = NULL;
    *(MenuCallbackSlot *)((u8 *)entity + 8) = u.s[0];
    SetEntitySpriteId(entity, 0x33808E1B, 1);
}

/* Allocates+inits a menu button with a caller-supplied direct sprite
 * hash via InitEntitySprite(entity, sprite_id, 0x3E8, x, y, 0). Installs
 * the temporary D_80012034 vtable, then calls AttachCursorToButton to
 * spawn the highlight sub-entity (stored at +0x100) and wire the shared
 * move-callbacks. Returns the button entity. */
Entity *InitMenuButtonWithSpriteId(Entity *entity, u32 spriteId, s16 x, s16 y) {
    InitEntitySprite(entity, spriteId, 0x3E8, x, y, 0);
    *(s32 *)((u8 *)entity + 0x18) = (s32)&D_80012034;
    AttachCursorToButton(entity);
    return entity;
}

/* Same as InitMenuButtonWithSpriteId but passes a sprite-*table* pointer
 * to InitEntityWithSprite (so the underlying sprite is resolved through
 * a lookup table -- used when the button art is animated/multi-frame).
 * Sets vtable=D_80012034 then AttachCursorToButton. */
Entity *InitMenuButtonWithSpritePtr(Entity *entity, void *spriteDef, s16 x, s16 y) {
    InitEntityWithSprite(entity, spriteDef, 0x3E8, x, y);
    *(s32 *)((u8 *)entity + 0x18) = (s32)&D_80012034;
    AttachCursorToButton(entity);
    return entity;
}

/* Builds the highlight child entity that follows a button. Allocates
 * 0x100 bytes from the BLB heap, inits via InitEntityWithSprite with
 * the D_8009CBE8 sprite table at z=0x7D0 and parent_(x,y)+(0x6A,0xE),
 * installs vtable D_8001208C and GetWorldPositionX/Y move-callbacks,
 * adds it to the sorted render list, and stores the child pointer at
 * parent+0x100 (with parent+0x104 cleared to "inactive"). Finally
 * installs matching GetWorldPositionX/Y move-callbacks on the parent.
 *
 * SHELVED: stack-marshal of the GetWorldPositionX/Y callback slots
 * doesn't reproduce the original frame layout. Original packs the 4
 * marshals into a 0x38 frame, reusing the s0 save-slot as scratch
 * (s0 holds the -1 marker after the first call); my C version emits
 * a 0x50 frame with separate scratch + 6 callee-saves. Likely needs
 * an explicit u32 -1 constant variable + manual slot reuse, or a
 * dedicated helper struct that pins the layout. Permuter candidate. */
INCLUDE_ASM("asm/nonmatchings/menu", AttachCursorToButton);

/* Cursor-focus enter handler for a plain menu button. Loads the
 * highlight child at parent+0x100, sets parent+0x104=1 (active),
 * installs MenuButtonCallback as the child's event handler, switches
 * the child sprite to 0x63848E59 (highlight flash), and queues
 * MenuSetEntityIdle2 at +0x98/+0x9C so the highlight settles back to
 * idle on the next callback-queue dispatch. */
/* Cursor-focus enter handler for a plain menu button. Loads the
 * highlight child at parent+0x100, sets parent+0x104=1 (active),
 * installs MenuButtonCallback as the child's event handler, switches
 * the child sprite to 0x63848E59 (highlight flash), and queues
 * MenuSetEntityIdle2 at +0x98/+0x9C so the highlight settles back to
 * idle on the next callback-queue dispatch.
 *
 * SHELVED: Quirk-5 lui-hoist scheduling diff. TripadSlot pins the 0x30
 * frame correctly, but cc1 in the original schedules `lui a1` /
 * `ori a1` for the sprite-id constant BEFORE the prologue saves to fill
 * delay slots, while modern cc1 sequences them after the saves. Same
 * class as MenuDeactivateButton. Closest draft kept in git history. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateButton);
#if 0
/* See git log for the C draft (TripadSlot u; ...; SetEntitySpriteId(
 * child, 0x63848E59, 1); ...). Frame matched at 0x30, all loads/stores
 * matched, only the constant-build hoist differs. */
#endif

/* Cursor-focus exit handler -- clears parent+0x104, zeroes the
 * highlight child's event-callback pair (+0x08/+0x0C), and switches
 * the child sprite to 0x39900619 (idle) via SetEntitySpriteId.
 *
 * SHELVED: 2-instruction scheduling-only diff — original hoists the
 * `lui $a1` for the 0x39900619 constant ahead of the sb/lw, mine emits
 * it just before the `ori`. Same registers, same instruction count,
 * Quirk-5-shape (decomp-permuter territory). Closest C draft kept in
 * git history. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuDeactivateButton);

/* Plays the confirm/select SFX (PlaySoundEffect(0x90810000, 0xA0, 0))
 * unconditionally and sets the gp-relative debounce byte D_800A6045 to
 * 1 so the *IfEnabled variants below know a sound has already been
 * issued this frame.
 *
 * SHELVED: writes to D_800A6045 which is a loose extern in this TU. cc1
 * emits `sb $v0, D_800A6045` (bare symbol), maspsx's gp_rel rewriter
 * only fires for symbols defined via .comm/.lcomm/.sdata in THIS TU,
 * so the store gets resolved as absolute `lui $at; sb $v0, %lo(...)`
 * (2 instructions instead of 1) and every later instruction shifts by
 * 4 bytes. See memories/repo/gp-rel-extern-blocker.md. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_PlayConfirmSound);

/* Builds a password-screen button via
 * InitEntitySprite(entity, 0x10094096, 0x3E8, x, y, 0). Sets vtable
 * D_80012034, runs AttachCursorToButton for the highlight, then
 * overrides vtable to D_80011FDC (password-button vtable). Stores
 * caller's type byte at +0x108 and the back-flag byte (5th arg) at
 * +0x109 -- these gate which Menu_Play*SoundIfEnabled helpers fire. */
Entity *InitMenuPasswordButton(Entity *entity, s16 x, s16 y, u8 typeByte, u8 backFlag) {
    InitEntitySprite(entity, 0x10094096, 0x3E8, x, y, 0);
    *(s32 *)((u8 *)entity + 0x18) = (s32)&D_80012034;
    AttachCursorToButton(entity);
    *(s32 *)((u8 *)entity + 0x18) = (s32)&D_80011FDC;
    *((u8 *)entity + 0x108) = typeByte;
    *((u8 *)entity + 0x109) = backFlag;
    return entity;
}

/* MenuActivateButton variant for buttons that own a label/value
 * sub-entity at parent+0x34 (e.g. password symbols or option labels).
 * Performs the standard activate flow (highlight sprite 0x63848E59,
 * queue MenuSetEntityIdle2), then HIDES the +0x34 sub-entity by
 * clearing its BasicEntity.active byte (+0x0A=0) while the flash
 * animation plays.
 *
 * SHELVED: Quirk-5 lui-hoist (same family as MenuActivateButton). */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateButtonWithReset);

/* MISLABELED -- has nothing to do with the FINN vehicle. This is the
 * deactivate counterpart to MenuActivateButtonWithReset: clears
 * parent+0x104, zeroes the highlight child's event-callback pair,
 * switches the child back to idle sprite 0x39900619, then re-shows the
 * +0x34 sub-entity by setting its active byte (+0x0A=1). Likely real
 * name: MenuDeactivateButtonWithReset.
 *
 * SHELVED: same Quirk-5 lui-hoist scheduling diff as MenuDeactivateButton
 * — original schedules `lui a1` for the 0x39900619 constant immediately
 * after the prologue, mine after the byte/load pair. Closest C draft in
 * git history. */
INCLUDE_ASM("asm/nonmatchings/menu", FINN_ClearSubentityState);

/* Conditional select-SFX helper. If entity+0x108 (the type byte set by
 * InitMenuPasswordButton) is nonzero, calls
 * PlaySoundEffect(0x90810000, 0xA0, 0) and copies that type byte into
 * the gp-relative D_800A6045 debounce flag. Lets only "real" buttons
 * (non-zero type) emit the click sound.
 *
 * SHELVED: writes to D_800A6045 (loose extern). Same gp_rel-extern
 * blocker as Menu_PlayConfirmSound. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_PlaySelectSoundIfEnabled);

/* Conditional confirm-SFX helper. If entity+0x109 (the back-flag byte
 * set by InitMenuPasswordButton) is nonzero, calls
 * PlaySoundEffect(0x90810000, 0xA0, 0) and writes 1 to D_800A6045.
 * Used so only the password screen's "back/confirm" button actually
 * fires the confirm sound.
 *
 * SHELVED: writes to D_800A6045 (loose extern). Same gp_rel-extern
 * blocker as Menu_PlayConfirmSound. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_PlayConfirmSoundIfEnabled);

/* Builds the level-picker button. Main body:
 * InitEntitySprite(0x10094096, z=0x3E8) -> vtable D_80012034 ->
 * AttachCursorToButton -> stash params (level-count byte at +0x10C,
 * position-table ptr at +0x108, current-index byte ptr at +0x110,
 * audio-settings ptr at +0x118) -> final vtable D_80011F84. Then
 * allocates a second child (the level-icon sprite) at the (x,y) read
 * from the position table for the currently-selected level, gives it
 * zOrder 0x4B0, stores it at +0x114 and pushes it onto the render list. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuLevelSelectButton);

/* Level-select counterpart of MenuActivateButtonWithReset -- same body:
 * flashes the highlight child at +0x100 with sprite 0x63848E59, queues
 * MenuSetEntityIdle2, and hides the level-icon sub-entity at parent
 * +0x34 by clearing its active byte (+0x0A=0). (The +0x34 entity is
 * separate from the +0x114 level-icon set up at init.)
 *
 * SHELVED: Quirk-5 lui-hoist (sprite-id constant). TripadSlot version
 * matches frame + body but the `lui a1, 0x6384` is hoisted to position
 * 1 by original cc1. Same as MenuActivateButton. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateLevelSelectButton);

/* Level-select deactivate -- byte-identical to FINN_ClearSubentityState:
 * clears parent+0x104, zeroes the highlight child's event-callback
 * pair, switches to idle sprite 0x39900619, and re-shows the +0x34
 * sub-entity (+0x0A=1).
 *
 * SHELVED: same Quirk-5 lui-hoist scheduling diff as the parent
 * FINN_ClearSubentityState body. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuDeactivateLevelSelectButton);

/* No-op stub @ 0x80075B7C (8 bytes: jr $ra; nop). Empty C body already
 * present -- likely an unused callback slot in the level-select button's
 * MenuCallbackTable (e.g. onCancel/onShoulder/onDpad ignored). */
void func_80075B7C(void) {
}

/* Level-select "cycle up" handler. If *(u8*)(entity+0x110) <
 * entity+0x10C - 1, increments the current-level index, calls
 * ApplyAudioSettings(entity+0x118), and plays the menu-cycle SFX
 * (PlaySoundEffect(0x686C1C97, 0xA0, 0)). Always recomputes the
 * level-icon entity (+0x114) worldX/worldY from the (x,y) table at
 * +0x108 indexed by new_index*4 so the icon slides to the next slot. */
void Menu_IncrementSelection(Entity *entity) {
    u8 *slider = *(u8 **)((u8 *)entity + 0x110);
    u8 max = *(u8 *)((u8 *)entity + 0x10C);
    u8 cur = *slider;
    if ((s32)cur < (s32)(max - 1)) {
        *slider = cur + 1;
        ApplyAudioSettings(*(void **)((u8 *)entity + 0x118));
        PlaySoundEffect(0x686C1C97, 0xA0, 0);
    }
    {
        u8 idx = **(u8 **)((u8 *)entity + 0x110);
        u8 *table = *(u8 **)((u8 *)entity + 0x108);
        u8 *display = *(u8 **)((u8 *)entity + 0x114);
        *(u16 *)(display + 0x68) = *(u16 *)(table + idx * 4 + 0);
    }
    {
        u8 idx = **(u8 **)((u8 *)entity + 0x110);
        u8 *table = *(u8 **)((u8 *)entity + 0x108);
        u8 *display = *(u8 **)((u8 *)entity + 0x114);
        *(u16 *)(display + 0x6A) = *(u16 *)(table + idx * 4 + 2);
    }
}

/* Cycle-down counterpart of Menu_IncrementSelection. If
 * *(u8*)(entity+0x110) > 0, decrements, calls ApplyAudioSettings, and
 * plays the 0x686C1C97 cycle SFX. Always repositions the level-icon
 * entity at +0x114 using the (x,y) lookup at +0x108 + new_index*4. */
void Menu_DecrementAndPlaySound(Entity *entity) {
    u8 *slider = *(u8 **)((u8 *)entity + 0x110);
    u8 cur = *slider;
    if (cur != 0) {
        *slider = cur - 1;
        ApplyAudioSettings(*(void **)((u8 *)entity + 0x118));
        PlaySoundEffect(0x686C1C97, 0xA0, 0);
    }
    {
        u8 idx = **(u8 **)((u8 *)entity + 0x110);
        u8 *table = *(u8 **)((u8 *)entity + 0x108);
        u8 *display = *(u8 **)((u8 *)entity + 0x114);
        *(u16 *)(display + 0x68) = *(u16 *)(table + idx * 4 + 0);
    }
    {
        u8 idx = **(u8 **)((u8 *)entity + 0x110);
        u8 *table = *(u8 **)((u8 *)entity + 0x108);
        u8 *display = *(u8 **)((u8 *)entity + 0x114);
        *(u16 *)(display + 0x6A) = *(u16 *)(table + idx * 4 + 2);
    }
}

/* Builds the skull-icon style multi-state button (used on the options
 * screen for difficulty/continues/sound-level selection). Scaffold
 * matches InitMenuLevelSelectButton: InitEntitySprite(0x10094096,0x3E8)
 * -> vtable D_80012034 -> AttachCursorToButton -> stash value-byte ptr
 * at +0x108 -> final vtable D_80011F2C. Then allocates a child via
 * InitEntitySprite(0x81100030,z=0x7D0) at (-0x40,-0x100), resolves its
 * TPage via GetTPage, sets zOrder 0x4B0, stores it at +0x10C, adds it
 * to the render list, then SetAnimationActive(child,0) and
 * SetAnimationFrameIndex(child, *(u8*)(+0x108)) so the icon starts on
 * the current value frame. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuSkullIconButton);

/* Skull-icon counterpart of MenuActivateButtonWithReset -- same body:
 * flashes the +0x100 highlight with sprite 0x63848E59, queues
 * MenuSetEntityIdle2, and hides the +0x34 sub-entity by clearing its
 * active byte.
 *
 * SHELVED: Quirk-5 lui-hoist (same family as MenuActivateButton). */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateSkullIconButton);

/* Skull-icon deactivate -- byte-identical to MenuDeactivateLevelSelectButton
 * / FINN_ClearSubentityState: clears parent+0x104, zeros the highlight
 * child's event-callback pair, switches to idle sprite 0x39900619, and
 * re-shows the +0x34 sub-entity (+0x0A=1).
 *
 * SHELVED: same Quirk-5 lui-hoist scheduling diff as the parent
 * FINN_ClearSubentityState body. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuDeactivateSkullIconButton);

/* No-op stub @ 0x80075F24 (8 bytes: jr $ra; nop). Empty C body already
 * present -- same role as func_80075B7C, an unused callback slot in the
 * skull-icon button's MenuCallbackTable. */
void func_80075F24(void) {
}

/* Skull-icon "cycle up" handler. Reads *(u8*)(entity+0x108),
 * increments it, wrapping to 0 if it was >= 4 (values cycle through
 * 0..4). Then SetAnimationFrameIndex(entity+0x10C, new_value) to update
 * the displayed icon frame and plays the menu-cycle SFX
 * (PlaySoundEffect(0x686C1C97, 0xA0, 0)). */
void Menu_CycleOptionAndPlaySound(Entity *entity) {
    u8 *counter = *(u8 **)((u8 *)entity + 0x108);
    u8 v = *counter;
    if (v < 4) *counter = v + 1; else *counter = 0;
    SetAnimationFrameIndex(*(void **)((u8 *)entity + 0x10C),
                           **(u8 **)((u8 *)entity + 0x108));
    PlaySoundEffect(0x686C1C97, 0xA0, 0);
}

/* Cycle-down counterpart of Menu_CycleOptionAndPlaySound. Decrements
 * *(u8*)(entity+0x108); if it was already 0, wraps back to 4 so the
 * range stays 0..4. Updates the icon via SetAnimationFrameIndex(
 * entity+0x10C, new_value) and plays the 0x686C1C97 cycle SFX. */
void Menu_DecrementCounter(Entity *entity) {
    u8 *counter = *(u8 **)((u8 *)entity + 0x108);
    u8 v = *counter;
    if (v == 0) v = 4; else v -= 1;
    *counter = v;
    SetAnimationFrameIndex(*(void **)((u8 *)entity + 0x10C),
                           **(u8 **)((u8 *)entity + 0x108));
    PlaySoundEffect(0x686C1C97, 0xA0, 0);
}
