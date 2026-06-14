#include "common.h"
#include "Game/entity.h"

extern void EntityProcessCallbackQueue(Entity *entity);
extern void EntityUpdateCallback(Entity *entity);

/* Allocates+inits the menu cursor SpriteEntity via InitEntityWithSprite
 * using the D_8009CBDC sprite table at z-order 0x7D0 (2000) and the
 * caller-supplied (x,y). Installs vtable D_800120AC, wires
 * GetWorldPositionX/Y as identity move-callbacks at +0x24/+0x2C (FSM
 * marker 0xFFFF0000 = direct call), then EntitySetState to the FSM
 * slot at D_800A6050/D_800A6054 to enter the cursor's idle tick state. */
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
 * the run animation kicks in when the timer expires. */
INCLUDE_ASM("asm/nonmatchings/menu", SetupMenuIdleAnimation);

/* Transition handler that swaps the menu character from idle to its
 * active running animation. Installs EntityUpdateCallback at the tick
 * slot and MenuEntityCallback as the event handler, switches sprite to
 * 0x0305A4F5, and queues SetupMenuIdleAnimation at +0x98/+0x9C so the
 * cycle re-enters idle once the run animation completes. */
INCLUDE_ASM("asm/nonmatchings/menu", InitRunnLevelEntity);

/* Allocates+inits a "button highlight" SpriteEntity from the D_8009CBE8
 * sprite table at z-order 0x7D0 (2000), with sign-extended s16 (x,y)
 * supplied by the caller. Installs vtable D_8001208C and the
 * GetWorldPositionX/Y identity move-callbacks at +0x24/+0x2C so the
 * sprite tracks its own world position. Returns the new entity. */
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
 * idle sprite. */
INCLUDE_ASM("asm/nonmatchings/menu", SetupMenuButtonAnimation);

/* Tail-call helper: clears the event-callback pair at +0x08/+0x0C and
 * switches the sprite id to 0x39900619 (the primary "idle"
 * button-highlight sprite) via SetEntitySpriteId. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuSetEntityIdle);

/* Variant of MenuSetEntityIdle that switches to the alternate idle
 * sprite 0x33808E1B; otherwise identical (clears event pair, calls
 * SetEntitySpriteId). Used as the post-flash resting state queued by
 * MenuActivateButton-family helpers. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuSetEntityIdle2);

/* Allocates+inits a menu button with a caller-supplied direct sprite
 * hash via InitEntitySprite(entity, sprite_id, 0x3E8, x, y, 0). Installs
 * the temporary D_80012034 vtable, then calls AttachCursorToButton to
 * spawn the highlight sub-entity (stored at +0x100) and wire the shared
 * move-callbacks. Returns the button entity. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuButtonWithSpriteId);

/* Same as InitMenuButtonWithSpriteId but passes a sprite-*table* pointer
 * to InitEntityWithSprite (so the underlying sprite is resolved through
 * a lookup table -- used when the button art is animated/multi-frame).
 * Sets vtable=D_80012034 then AttachCursorToButton. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuButtonWithSpritePtr);

/* Builds the highlight child entity that follows a button. Allocates
 * 0x100 bytes from the BLB heap, inits via InitEntityWithSprite with
 * the D_8009CBE8 sprite table at z=0x7D0 and parent_(x,y)+(0x6A,0xE),
 * installs vtable D_8001208C and GetWorldPositionX/Y move-callbacks,
 * adds it to the sorted render list, and stores the child pointer at
 * parent+0x100 (with parent+0x104 cleared to "inactive"). Finally
 * installs matching GetWorldPositionX/Y move-callbacks on the parent. */
INCLUDE_ASM("asm/nonmatchings/menu", AttachCursorToButton);

/* Cursor-focus enter handler for a plain menu button. Loads the
 * highlight child at parent+0x100, sets parent+0x104=1 (active),
 * installs MenuButtonCallback as the child's event handler, switches
 * the child sprite to 0x63848E59 (highlight flash), and queues
 * MenuSetEntityIdle2 at +0x98/+0x9C so the highlight settles back to
 * idle on the next callback-queue dispatch. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateButton);

/* Cursor-focus exit handler -- clears parent+0x104, zeroes the
 * highlight child's event-callback pair (+0x08/+0x0C), and switches
 * the child sprite to 0x39900619 (idle) via SetEntitySpriteId. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuDeactivateButton);

/* Plays the confirm/select SFX (PlaySoundEffect(0x90810000, 0xA0, 0))
 * unconditionally and sets the gp-relative debounce byte D_800A6045 to
 * 1 so the *IfEnabled variants below know a sound has already been
 * issued this frame. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_PlayConfirmSound);

/* Builds a password-screen button via
 * InitEntitySprite(entity, 0x10094096, 0x3E8, x, y, 0). Sets vtable
 * D_80012034, runs AttachCursorToButton for the highlight, then
 * overrides vtable to D_80011FDC (password-button vtable). Stores
 * caller's type byte at +0x108 and the back-flag byte (5th arg) at
 * +0x109 -- these gate which Menu_Play*SoundIfEnabled helpers fire. */
INCLUDE_ASM("asm/nonmatchings/menu", InitMenuPasswordButton);

/* MenuActivateButton variant for buttons that own a label/value
 * sub-entity at parent+0x34 (e.g. password symbols or option labels).
 * Performs the standard activate flow (highlight sprite 0x63848E59,
 * queue MenuSetEntityIdle2), then HIDES the +0x34 sub-entity by
 * clearing its BasicEntity.active byte (+0x0A=0) while the flash
 * animation plays. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateButtonWithReset);

/* MISLABELED -- has nothing to do with the FINN vehicle. This is the
 * deactivate counterpart to MenuActivateButtonWithReset: clears
 * parent+0x104, zeroes the highlight child's event-callback pair,
 * switches the child back to idle sprite 0x39900619, then re-shows the
 * +0x34 sub-entity by setting its active byte (+0x0A=1). Likely real
 * name: MenuDeactivateButtonWithReset. */
INCLUDE_ASM("asm/nonmatchings/menu", FINN_ClearSubentityState);

/* Conditional select-SFX helper. If entity+0x108 (the type byte set by
 * InitMenuPasswordButton) is nonzero, calls
 * PlaySoundEffect(0x90810000, 0xA0, 0) and copies that type byte into
 * the gp-relative D_800A6045 debounce flag. Lets only "real" buttons
 * (non-zero type) emit the click sound. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_PlaySelectSoundIfEnabled);

/* Conditional confirm-SFX helper. If entity+0x109 (the back-flag byte
 * set by InitMenuPasswordButton) is nonzero, calls
 * PlaySoundEffect(0x90810000, 0xA0, 0) and writes 1 to D_800A6045.
 * Used so only the password screen's "back/confirm" button actually
 * fires the confirm sound. */
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
 * separate from the +0x114 level-icon set up at init.) */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateLevelSelectButton);

/* Level-select deactivate -- byte-identical to FINN_ClearSubentityState:
 * clears parent+0x104, zeroes the highlight child's event-callback
 * pair, switches to idle sprite 0x39900619, and re-shows the +0x34
 * sub-entity (+0x0A=1). */
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
INCLUDE_ASM("asm/nonmatchings/menu", Menu_IncrementSelection);

/* Cycle-down counterpart of Menu_IncrementSelection. If
 * *(u8*)(entity+0x110) > 0, decrements, calls ApplyAudioSettings, and
 * plays the 0x686C1C97 cycle SFX. Always repositions the level-icon
 * entity at +0x114 using the (x,y) lookup at +0x108 + new_index*4. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_DecrementAndPlaySound);

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
 * active byte. */
INCLUDE_ASM("asm/nonmatchings/menu", MenuActivateSkullIconButton);

/* Skull-icon deactivate -- byte-identical to MenuDeactivateLevelSelectButton
 * / FINN_ClearSubentityState: clears parent+0x104, zeros the highlight
 * child's event-callback pair, switches to idle sprite 0x39900619, and
 * re-shows the +0x34 sub-entity (+0x0A=1). */
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
INCLUDE_ASM("asm/nonmatchings/menu", Menu_CycleOptionAndPlaySound);

/* Cycle-down counterpart of Menu_CycleOptionAndPlaySound. Decrements
 * *(u8*)(entity+0x108); if it was already 0, wraps back to 4 so the
 * range stays 0..4. Updates the icon via SetAnimationFrameIndex(
 * entity+0x10C, new_value) and plays the 0x686C1C97 cycle SFX. */
INCLUDE_ASM("asm/nonmatchings/menu", Menu_DecrementCounter);
