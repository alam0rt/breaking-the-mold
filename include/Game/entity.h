#ifndef ENTITY_H
#define ENTITY_H

#include "common.h"
#include "Game/input_state.h"

/* =============================================================================
 * ENTITY HIERARCHY
 *
 * Clean-room RE note: all struct/field names are inferred working labels,
 * not original source names. Keep names conservative when fields are overlaid
 * or only partially traced.
 * 
 * The game uses THREE different entity struct layouts, each with its own
 * vtable location and field organization:
 * 
 * 1. BasicEntity (0x10 bytes) - InitBasicEntityWithVtable @ 0x8001543c
 *    Minimal struct for simple UI helpers. vtable @ +0x0C
 * 
 * 2. MenuEntityBase (0x1C bytes) - InitMenuEntityWithVtable @ 0x80019748
 *    Base for menu/UI elements. vtable @ +0x18. Callers extend with more fields.
 * 
 * 3. Entity (0x80 bytes) - InitEntityStruct @ 0x8001a0c8
 *    Full game entity with callbacks, physics, bounds. vtable @ +0x18
 *
 * Extended hierarchy (allocation sizes from SpawnPlayerAndEntities @ 0x8007DF38):
 *
 *   Entity (0x80)
 *   └── SpriteEntity (0x100)  - Entity + animation state machine
 *       ├── PlayerEntity     (0x1B4) - main platformer player
 *       ├── FinnPlayerEntity (0x114) - vehicle/boat player
 *       ├── RunnPlayerEntity (0x110) - auto-scroller runner
 *       ├── SoarPlayerEntity (0x128) - flying player
 *       ├── GlidePlayerEntity (0x11C alloc) - glide variant; distinct struct
 *       │                                     TBD, currently uses SoarPlayerEntity
 *       └── PathEnemyEntity  (0x120, 0x130 alloc) - path-following enemy
 *
 * All extended types share the Entity base (0x00-0x7F) and the SpriteEntity
 * animation fields (0x80-0xFF); type-specific fields begin at 0x100.
 * ============================================================================= */

/* -----------------------------------------------------------------------------
 * BasicEntity (0x10 = 16 bytes)
 * 
 * Base struct for renderable objects in the z-sorted render list.
 * Initialized by InitBasicEntityWithVtable @ 0x8001543c.
 * 
 * This is a POLYMORPHIC struct - field meanings depend on vtable:
 * - SpriteRenderContext (vtable 0x80010384): widthOrU/heightOrV = dimensions
 * - UI/Cursor entities (vtable 0x80010b98): widthOrU/heightOrV = VRAM UV coords
 * - Colored overlays (vtable 0x80010aa8): fields 0x04-0x06 unused (full screen)
 * 
 * Callers extend this struct for type-specific data (e.g., +0x40 for RGBA).
 * Render list sorted by zOrder (AddToXPositionList @ 0x8002107c).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ s16  screenX;        /* Screen X position for rendering (calculated) */
    /* 0x02 */ s16  screenY;        /* Screen Y position for rendering (calculated) */
    /* 0x04 */ s16  widthOrU;       /* Width (sprites) or VRAM texture U (UI) */
    /* 0x06 */ s16  heightOrV;      /* Height (sprites) or VRAM texture V (UI) */
    /* 0x08 */ s16  zOrder;         /* Z-order for render list (lower = behind) */
    /* 0x0A */ u8   active;         /* Enable flag (init to 1) */
    /* 0x0B */ u8   pad0B;
    /* 0x0C */ void *vtable;        /* Callback vtable (determines field interpretation) */
} BasicEntity;  /* Size: 0x10 */

/* -----------------------------------------------------------------------------
 * MenuEntityBase (0x1C = 28 bytes minimum)
 * 
 * Base entity for menu/UI elements like pause menu shadows.
 * Initialized by InitMenuEntityWithVtable @ 0x80019748
 * Uses vtable at 0x800104ac
 * 
 * IMPORTANT: This is just the BASE - callers extend it significantly:
 *   - CreateMenuEntities: Uses up to offset 0xAB (holds 0x28 child pointers!)
 *   - InitMenuCursorEntity: Uses up to offset 0x2C  
 *   - CreateFadeOverlayEntity: Uses up to offset 0x22
 * 
 * The actual allocation size varies per menu entity type.
 * See CreateMenuEntities @ 0x800281a4 for the largest example.
 * 
 * Fields 0x00-0x0F are FSM callback pairs (same as Entity):
 *   - Zeroed by init, but callers immediately set tickMarker=0xFFFF0000, tickCallback
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* FSM State Machine (0x00-0x0F) - same as Entity */
    /* 0x00 */ s16  tickMarker;     /* FSM tick marker low (0xFFFF0000 = direct call) */
    /* 0x02 */ s16  tickMarkerHi;   /* FSM tick marker high (0xFFFF for direct call) */
    /* 0x04 */ void *tickCallback;  /* Per-frame update function pointer */
    /* 0x08 */ s16  eventMarker;    /* FSM event marker low (0xFFFF0000 = direct call) */
    /* 0x0A */ s16  eventMarkerHi;  /* FSM event marker high */
    /* 0x0C */ void *eventCallback; /* Event handler function pointer */
    /* Entity Info (0x10-0x17) */
    /* 0x10 */ s16  zOrder;         /* Z-order for render list sorting (param_2 from init) */
    /* 0x12 */ s16  reserved12;     /* Reserved/unused (zeroed by init, not used) */
    /* 0x14 */ u8   active;         /* Enable flag (init to 1) */
    /* 0x15 */ u8   pad15;
    /* 0x16 */ u8   pad16;
    /* 0x17 */ u8   pad17;
    /* 0x18 */ void *vtable;        /* Callback vtable (0x800104ac) */
} MenuEntityBase;  /* Size: 0x1C (28 bytes) */

/* -----------------------------------------------------------------------------
 * Entity (Full Game Entity - 0x80 = 128 bytes)
 * 
 * Runtime base entity structure for game objects. Extended entity types
 * (Player, HUD, enemies, etc.) allocate more memory and add type-specific
 * fields after offset 0x78.
 * 
 * Allocation size is stored at +0x10 and varies by entity type:
 *   - Base entities: 0x78-0x80
 *   - Extended enemies: 0x100-0x130
 *   - Player entity: 0x3E8 (1000 bytes)
 *   - HUD/manager: 0x7530 (30000 bytes)
 * 
 * Initialized by InitEntityStruct @ 0x8001a0c8
 * ----------------------------------------------------------------------------- */

typedef struct Entity Entity;

typedef void (*EntityCallback)(Entity *entity);

/* Coordinate-transform callback for the moveX/moveY FSM slots: receives
 * (entity + markerLo [+ table entry arg], coordinate) and returns the
 * transformed coordinate. Verified in EntityBroadcastPointCollision
 * @ 0x8001B72C. */
typedef s16 (*EntityCoordCallback)(void *target, s16 coord);

/* FSM marker encoding (applies to ALL marker+callback pairs; decoded from
 * DispatchEventToCollidingEntity @ 0x800226F8 and EntityBroadcastPointCollision
 * @ 0x8001B72C):
 *   hi s16 == 0 : no callback installed
 *   hi s16 <  0 : call the fn field directly with (entity + lo s16)
 *   hi s16 >  0 : index into an 8-byte slot table located at
 *                 *(entity + (s16)fn-field); entry = {s16 arg, fn};
 *                 call fn(entity + lo + arg)
 * 0xFFFF0000 is therefore "direct call, offset 0" - the common case. */

struct Entity {
    /* State Machine - Tick System (0x00-0x0F) */
    /* 0x00 */ s32             tickMarker;       /* FSM marker (see encoding above) */
    /* 0x04 */ EntityCallback  tickCallback;     /* Per-frame update function */
    /* 0x08 */ s32             eventMarker;      /* FSM marker (init 0xFFFF0000) */
    /* 0x0C */ EntityCallback  eventCallback;    /* Event handler (init StubReturnZero) */
    
    /* Entity Info (0x10-0x17) */
    /* 0x10 */ s16             allocSize;        /* Total allocation size for this entity */
    /* 0x12 */ s16             collisionMask;    /* Entity collision mask flags */
    /* 0x14 */ u8              active;           /* Active flag (init to 1) */
    /* 0x15 */ u8              pad15;            /* Padding (extended types may use) */
    /* 0x16 */ u8              pad16;            /* Padding (extended types may use) */
    /* 0x17 */ u8              pad17;            /* Padding (extended types may use) */
    
    /* Collision System (0x18-0x1F) */
    /* 0x18 */ void           *collisionVtable;  /* Collision handler vtable */
    /* 0x1C */ s32             renderMarker;     /* Render callback marker */
    
    /* Render Callback (0x20-0x23) */
    /* 0x20 */ EntityCallback  renderCallback;   /* Render function */
    
    /* Movement / coordinate-transform FSM slots (0x24-0x33).
     * Two more standard [marker, fn] pairs (NOT s16 triplets as previously
     * documented). The callbacks transform worldX/worldY before collision
     * and rendering (e.g. platform riding). 93 install sites each - see
     * docs/analysis/callback-install-map.md. Verified via
     * EntityBroadcastPointCollision @ 0x8001B72C and InitEntityStruct's
     * word-sized zeroing of +0x28/+0x30. */
    /* 0x24 */ s32                 moveMarkerX;   /* FSM marker (see encoding above) */
    /* 0x28 */ EntityCoordCallback moveCallbackX; /* worldX transform callback */
    /* 0x2C */ s32                 moveMarkerY;   /* FSM marker */
    /* 0x30 */ EntityCoordCallback moveCallbackY; /* worldY transform callback */
    
    /* Sprite Context (0x34-0x37) */
    /* 0x34 */ void           *spriteContext;    /* Sprite render context pointer */
    
    /* Render Bounds (0x38-0x3F) */
    /* 0x38 */ s16             renderOffsetX;    /* Render X offset from position */
    /* 0x3A */ s16             renderOffsetY;    /* Render Y offset from position */
    /* 0x3C */ s16             renderWidth;      /* Render width */
    /* 0x3E */ s16             renderHeight;     /* Render height */
    
    /* Hitbox (0x40-0x47) */
    /* 0x40 */ s16             hitboxOffsetX;    /* Hitbox X offset from position */
    /* 0x42 */ s16             hitboxOffsetY;    /* Hitbox Y offset from position */
    /* 0x44 */ s16             hitboxWidth;      /* Hitbox width */
    /* 0x46 */ s16             hitboxHeight;     /* Hitbox height */
    
    /* Screen Bounds - Calculated (0x48-0x4F) */
    /* 0x48 */ s16             screenX1;         /* Screen left (calculated) */
    /* 0x4A */ s16             screenY1;         /* Screen top (calculated) */
    /* 0x4C */ s16             screenX2;         /* Screen right (calculated) */
    /* 0x4E */ s16             screenY2;         /* Screen bottom (calculated) */
    
    /* Scale Factors - 16.16 Fixed Point (0x50-0x67) */
    /* All initialized to 0x10000 (1.0) by InitEntityStruct */
    /* Used by SetupEntityScaleCallbacks @ 0x8001c364 for shrink/grow powerup */
    /* 0x50 */ s32             scaleRender;      /* Render scale (applied to worldX/Y for final position) */
    /* 0x54 */ s32             scaleRender2;     /* Secondary render scale */
    /* 0x58 */ s32             scalePowerupX;    /* X scale from powerup (shrink/grow) */
    /* 0x5C */ s32             scalePowerupY;    /* Y scale from powerup (shrink/grow) */
    /* 0x60 */ s32             scaleParallaxX;   /* X parallax scale (g_GameStatePtr+0x44) */
    /* 0x64 */ s32             scaleParallaxY;   /* Y parallax scale (g_GameStatePtr+0x46) */
    
    /* Position (0x68-0x6F) */
    /* 0x68 */ s16             worldX;           /* World X position (pixels) */
    /* 0x6A */ s16             worldY;           /* World Y position (pixels) */
    /* 0x6C */ s16             velocityX;        /* X velocity */
    /* 0x6E */ s16             velocityY;        /* Y velocity */
    
    /* Target Position (0x70-0x73) */
    /* 0x70 */ s16             targetX;          /* Target X for movement */
    /* 0x72 */ s16             targetY;          /* Target Y for movement */
    
    /* Flags (0x74-0x77) */
    /* 0x74 */ u8              facing;           /* Facing direction: 0=right, 1=left (VERIFIED via
                                                  * PlayerCallback_JumpInputAndCounters @ 0x800602E0:
                                                  * Left button sets velocityX=-max AND facing=1) */
    /* 0x75 */ u8              flipY;            /* Vertical flip flag */
    /* 0x76 */ u8              textureDirty;     /* Texture needs upload (set when sprite changes) */
    /* 0x77 */ u8              boundsValid;      /* Screen bounds are valid (1=valid, skip recalculation) */
    
    /* Sprite-render sub-struct base (0x78-0x7F). Previously documented as
     * moveCallbackY/moveCallbackX - wrong (zero fn installs; real move
     * callbacks are the FSM pairs at +0x24/+0x2C).
     * VERIFIED via UpdateEntityRender @ 0x8001D988: +0x78 is the frame
     * metadata table (0x24 bytes/frame: +0xA width, +0xC height/duration)
     * and &entity->pFrameTable is passed as the base of an embedded
     * sprite-render context to RenderSprite/GetFrameMetadata/
     * DecodeRLESpriteChecked. The animation state continues at 0x80-0xFF
     * in sprite entities (alloc >= 0x100) - see PlayerEntity in Ghidra. */
    /* 0x78 */ void           *pFrameTable;     /* Frame metadata table (0x24/frame) */
    /* 0x7C */ void           *pPaletteData;    /* Palette/CLUT data uploaded by UploadEntityTextureIfDirty */
};  /* Size: 0x80 (128 bytes) */

/* -----------------------------------------------------------------------------
 * SpriteEntity (0x100 = 256 bytes)
 *
 * Entity extended with the full sprite animation state machine. This is the
 * common base for every animated game object: the player variants, enemies,
 * pickups, bosses, particles, etc. Initialized by InitEntitySprite
 * @ 0x8001C720 (152 xrefs), which clears 0x44C bytes total - the extra
 * space past 0x100 is workspace claimed by the extended types.
 *
 * Layout mirrors Ghidra /Skullmonkeys/SpriteEntity exactly. The animation
 * fields start at 0x88; 0x80-0x87 is unobserved (zeroed at init, no reader
 * identified yet). Gaps are reproduced as explicit _pad arrays so
 * sizeof(SpriteEntity) == 0x100.
 *
 * Frame flow (UpdateEntityRender @ 0x8001D988, SetEntitySpriteId @ 0x8001D080):
 *   pending* fields are latched by SetEntitySpriteId and committed into the
 *   current* fields on the next tick; frameCountdown counts down from
 *   frameRateDivisor and advances currentFrame toward targetFrame, looping
 *   back to loopFrame when animLoopFlag is set.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ Entity   base;               /* Entity header (FSM, physics, bounds) */

    /* 0x80 */ u8       _pad80[8];          /* Unobserved (zeroed by InitEntitySprite) */

    /* Frame metadata (0x88-0x93) */
    /* 0x88 */ u16      frameCount;         /* Total frame count in current sprite */
    /* 0x8A */ u8       spriteLookupByte;   /* Low byte from sprite lookup entry+0x08; copied to cache metadata */
    /* 0x8B */ u8       spriteContextValid; /* Embedded sprite context valid/decode-enabled byte */
    /* 0x8C */ void    *pFrameData;         /* Current frame data pointer */
    /* 0x90 */ void    *pSpriteAsset;       /* Sprite asset base pointer */
    /* 0x94 */ void    *sequenceTable;      /* Animation callback sequence table; entries are [marker, fn] */

    /* Animated state / callback queue (0x98-0xAF).
     * Verified through StartAnimationSequence @ 0x8001E790,
     * StepAnimationSequence @ 0x8001E7B8, EntityProcessCallbackQueue
     * @ 0x8001E928, EntitySetState @ 0x8001EAAC, and
     * EntitySetCallback @ 0x8001EC18. EntitySetCallback installs the
     * exit/finalizer hook at +0xA8/+0xAC; replacing it or changing state
     * dispatches the existing hook first. */
    /* 0x98 */ s32      queuedStateMarker;   /* Queued next-state marker consumed by EntityProcessCallbackQueue */
    /* 0x9C */ void    *queuedStateCallback; /* Queued next-state callback */
    /* 0xA0 */ s32      activeStateMarker;   /* Currently dispatched state/sequence marker */
    /* 0xA4 */ void    *activeStateCallback; /* Currently dispatched state/sequence callback */
    /* 0xA8 */ s32      exitCallbackMarker;  /* Exit/finalizer marker run before replacement/state changes */
    /* 0xAC */ void    *exitCallback;        /* Exit/finalizer callback (cleanup/follow-up hook) */

    /* Pixel buffer / per-frame motion (0xB0-0xBB) */
    /* 0xB0 */ void    *pPixelBuffer;       /* Decoded pixel data buffer */
    /* 0xB4 */ s32      frameMotionX;       /* Per-frame X motion vector (16.16 fixed) */
    /* 0xB8 */ s32      frameMotionY;       /* Per-frame Y motion vector (16.16 fixed) */

    /* Pending sprite change (0xBC-0xCB) - latched by SetEntitySpriteId */
    /* 0xBC */ u32      pendingSpriteId;    /* Sprite ID queued for change */
    /* 0xC0 */ s16      pendingFrame;       /* Pending start frame */
    /* 0xC2 */ s16      _padC2;
    /* 0xC4 */ s16      pendingLoopFrame;   /* Pending loop start frame */
    /* 0xC6 */ s16      _padC6;
    /* 0xC8 */ s16      pendingTargetFrame; /* Pending target/end frame */
    /* 0xCA */ s16      _padCA;

    /* Current sprite state (0xCC-0xDF) */
    /* 0xCC */ u32      currentSpriteId;    /* Active sprite bank ID (hash) */
    /* 0xD0 */ u32      displayedSpriteId;  /* Last rendered sprite ID */
    /* 0xD4 */ s32      pixelBufferSize;
    /* 0xD8 */ u16      curSpriteFrameCount;/* Frame count of current sprite */
    /* 0xDA */ s16      currentFrame;       /* Current animation frame index */
    /* 0xDC */ s16      loopFrame;          /* Frame to loop back to */
    /* 0xDE */ s16      targetFrame;        /* Target frame (stop/loop point) */

    /* Animation control (0xE0-0xFD) */
    /* 0xE0 */ u16      animChangeFlags;    /* Flags for pending animation changes */
    /* 0xE2 */ u16      sequenceStep;       /* Current index into sequenceTable */
    /* 0xE4 */ u16      sequenceLength;     /* Number of entries in sequenceTable */
    /* 0xE6 */ s16      frameDeltaX;        /* Raw signed X delta from current frame metadata */
    /* 0xE8 */ s16      frameDeltaY;        /* Raw signed Y delta from current frame metadata */
    /* 0xEA */ u16      nextFrame;          /* Next frame to display */
    /* 0xEC */ u16      frameRateDivisor;   /* Frame rate divisor (speed control) */
    /* 0xEE */ u16      frameCountdown;     /* Countdown to next frame advance */
    /* 0xF0 */ u8       animDirection;      /* 0=forward, 1=reverse */
    /* 0xF1 */ u8       animLoopFlag;       /* Loop animation on completion */
    /* 0xF2 */ u8       animActive;         /* Animation currently playing */
    /* 0xF3 */ u8       pendingDirection;   /* Pending direction change */
    /* 0xF4 */ u8       pendingLoopFlag;    /* Pending loop mode */
    /* 0xF5 */ u8       pendingAnimActive;  /* Pending active state */
    /* 0xF6 */ u8       visibility;         /* Visibility flag (0=visible) */
    /* 0xF7 */ u8       staticSpriteFlag;   /* 0=animated, 1=static sprite mode */
    /* 0xF8 */ u8       needsDecodeFlag;    /* Sprite needs RLE decode */
    /* 0xF9 */ u8       decodeStateFlag;    /* Decode in progress */
    /* 0xFA */ u8       cachedFacing;       /* Cached facing for flip detection */
    /* 0xFB */ u8       cachedFlipY;        /* Cached vertical flip */
    /* 0xFC */ u8       bufferClearedFlag;  /* Pixel buffer has been cleared */
    /* 0xFD */ u8       alwaysRenderFlag;   /* Force render even if offscreen */
    /* 0xFE */ u8       doubleFrameDelay;   /* Non-zero doubles frame_delay when loading frame metadata */
    /* 0xFF */ u8       unusedFF;
} SpriteEntity;  /* Size: 0x100 (256 bytes) */

/* -----------------------------------------------------------------------------
 * PlayerEntity (0x1B4 = 436 bytes)
 *
 * The standard platformer player (also reused by the GLIDE variant via
 * CreateGlidePlayerEntity @ 0x8006EDB8 with a 0x11C allocation).
 * Created by CreatePlayerEntity; allocated 0x1B4 from
 * SpawnPlayerAndEntities @ 0x8007DF38, but allocSize is set to 0x3E8 -
 * the tail is scratch space.
 *
 * Field offsets/names mirror Ghidra /Skullmonkeys/PlayerEntity. Unnamed
 * gaps are explicit _pad arrays. velocityX/Y_fixed are the 16.16
 * fixed-point physics velocities, distinct from the s16 velocity fields
 * in the Entity base (Ghidra names them velocityX/velocityY).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x000 */ SpriteEntity sprite;            /* Entity + animation state machine */

    /* Input system (0x100-0x10B) */
    /* 0x100 */ InputState *pInput;             /* Controller input pointer */
    /* 0x104 */ s32      inputStateMarker;      /* Input FSM marker */
    /* 0x108 */ void    *inputStateCallback;    /* Input handler callback */
    /* 0x10C */ u8       _pad10C[4];

    /* Physics (0x110-0x11F) */
    /* 0x110 */ s32      velocityY_fixed;       /* Y velocity (16.16 fixed) */
    /* 0x114 */ s32      velocityX_fixed;       /* X velocity (16.16 fixed) */
    /* 0x118 */ s32      cushionVelY;           /* Landing cushion velocity */
    /* 0x11C */ u8       landingTimer;          /* Landing recovery timer */
    /* 0x11D */ u8       _pad11D;
    /* 0x11E */ u8       bounceLockTimer;       /* Bounce/quick-turn lockout timer */
    /* 0x11F */ u8       jumpHoldCounter;       /* Jump button hold duration */

    /* Speed parameters (0x120-0x133) */
    /* 0x120 */ s32      altSpeed;              /* Alternate movement speed */
    /* 0x124 */ s32      maxVelocity;           /* Maximum velocity cap */
    /* 0x128 */ u8       invincibilityTimer;    /* Invincibility countdown (damage flash) */
    /* 0x129 */ u8       _pad129[11];

    /* State flags (0x134-0x143) */
    /* 0x134 */ u8       specialMoveQueued;     /* Queued special/teleport movement input */
    /* 0x135 */ u8       specialMoveMode;       /* Special movement branch selector */
    /* 0x136 */ s16      apexVelocity;          /* Jump apex velocity threshold */
    /* 0x138 */ u8       _pad138[4];
    /* 0x13C */ u8       timer13C;              /* Multi-purpose timer */
    /* 0x13D */ u8       _pad13D[7];

    /* Powerup system (0x144-0x14F) */
    /* 0x144 */ s16      powerupTimer;          /* Powerup duration countdown */
    /* 0x146 */ u8       _pad146[6];
    /* 0x14C */ void    *hudEntity;             /* HUD entity pointer (0x7530 alloc) */
    /* 0x150 */ u8       _pad150[6];

    /* Jump/movement (0x156-0x163) */
    /* 0x156 */ s16      jumpParam;             /* Jump force parameter */
    /* 0x158 */ u8       _pad158;
    /* 0x159 */ u8       pendingStateChange;    /* Queued state transition flag */
    /* 0x15A */ u8       currentRGB[3];         /* Current entity RGB tint */
    /* 0x15D */ u8       baseRGB[3];            /* Base/default RGB tint */
    /* 0x160 */ s16      pushX;                 /* External push force X */
    /* 0x162 */ s16      pushY;                 /* External push force Y */
    /* 0x164 */ u8       _pad164[4];

    /* Linked entities (0x168-0x173) */
    /* 0x168 */ void    *haloEntity;            /* Halo visual effect entity */
    /* 0x16C */ void    *glideEntity;           /* Glide cape entity */
    /* 0x170 */ u8       groundedFlag;          /* On ground (0=airborne, 1=grounded) */
    /* 0x171 */ u8       _pad171[3];

    /* Audio / rendering (0x174-0x17D) */
    /* 0x174 */ s32      soundHandle;           /* Active SPU sound handle */
    /* 0x178 */ u8       disableScale;          /* Disable scale effects */
    /* 0x179 */ u8       _pad179[4];
    /* 0x17D */ u8       rgbCooldown;           /* RGB effect cooldown timer */
    /* 0x17E */ u8       _pad17E[40];

    /* Scroll/camera (0x1A6-0x1A9) */
    /* 0x1A6 */ s16      scrollFlagX;           /* Camera scroll influence X */
    /* 0x1A8 */ s16      scrollFlagY;           /* Camera scroll influence Y */
    /* 0x1AA */ u8       _pad1AA[4];

    /* Damage/effects (0x1AE-0x1B3) */
    /* 0x1AE */ u8       damageFlag;            /* Taking damage flag */
    /* 0x1AF */ u8       particleFlag;          /* Particle effect active */
    /* 0x1B0 */ u8       shrinkFlag;            /* Shrink powerup active */
    /* 0x1B1 */ u8       _pad1B1[2];
    /* 0x1B3 */ u8       gameMode;              /* Current game mode identifier */
} PlayerEntity;  /* Size: 0x1B4 (436 bytes) */

/* -----------------------------------------------------------------------------
 * FinnPlayerEntity (0x114 = 276 bytes)
 *
 * Tank-control vehicle player for water/fish levels (level flag 0x0400).
 * Created by CreateFinnPlayerEntity @ 0x80074100. Spawns a secondary
 * wake/shadow sprite with the same sprite ID at z-order 0x3E9.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ InputState   *pInput;                      /* Controller input pointer */
    /* 0x104 */ void         *wakeEntity_or_velocityX;     /* Wake child pointer during init; X velocity accumulator during movement */
    /* 0x108 */ void         *velocityY_or_stateCallback;  /* Y velocity accumulator or callback/FSM overlay */
    /* 0x10C */ s16           rotationAngle;               /* FINN heading angle */
    /* 0x10E */ u8            rotationSpriteBucket;        /* Cached 16-way heading sprite bucket */
    /* 0x10F */ s8            rotationVelocity;            /* Signed angular velocity */
    /* 0x110 */ s32           soundHandle_or_inputFlags;   /* Active SPU voice handle or packed input flags */
} FinnPlayerEntity;  /* Size: 0x114 (276 bytes) */

/* -----------------------------------------------------------------------------
 * RunnPlayerEntity (0x110 = 272 bytes)
 *
 * Auto-scroller runner player for RUNN levels (level flag 0x0100).
 * Created by CreateRunnPlayerEntity @ 0x80073934. No secondary sprite.
 * Controls: Triangle=adjust left, X=adjust right, D-Pad=jump.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ InputState *pInput;     /* Controller input pointer */
    /* 0x104 */ s32         velocityX;  /* Runner X velocity (16.16 fixed) */
    /* 0x108 */ s32         velocityY;  /* Runner Y velocity (16.16 fixed) */
    /* 0x10C */ s32         spuVoice;   /* SPU voice handle (init -1) */
} RunnPlayerEntity;  /* Size: 0x110 (272 bytes) */

/* -----------------------------------------------------------------------------
 * SoarPlayerEntity (0x128 = 296 bytes)
 *
 * Flying player for SOAR levels (level flag 0x0010). Created by
 * CreateSoarPlayerEntity @ 0x80070D68; spawns a secondary sprite (z=2000)
 * and a particle entity (z=0x3D4). RGB tint inits to gray (0x40,0x40,0x40);
 * camera is offset -0x80 in Y for the aerial view.
 *
 * NOTE: CreateGlidePlayerEntity @ 0x8006EDB8 currently reuses this layout
 * in Ghidra with a smaller 0x11C allocation; a distinct GlidePlayerEntity
 * struct is TBD (see docs/ghidra/follow-ups-2026-01-20.md).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ InputState *pInput;       /* Controller input pointer */
    /* 0x104 */ s32         nFlightSpeed; /* Current flight speed (0 at init) */
    /* 0x108 */ u8          _pad108[4];
    /* 0x10C */ s32         handle10C;    /* SPU handle (init -1) */
    /* 0x110 */ u8          _pad110[2];
    /* 0x112 */ u8          stateTimer;   /* State transition timer */
    /* 0x113 */ u8          _pad113[5];
    /* 0x118 */ u8          gravityHoldTimer;     /* Set to 5 by bounce/gravity entry; decremented while applying gravity response */
    /* 0x119 */ u8          forcedGravityTimer;   /* Auxiliary gravity countdown; no non-zero producer found in checked SOAR/platform range */
    /* 0x11A */ u8          jumpTransitionLock;   /* 0 = jump enters bounce/gravity state; 1 = jump refreshes jumpHoldTimer */
    /* 0x11B */ u8          jumpHoldTimer;        /* 10-frame jump/confirm hold timer; decremented by platform tick */
    /* 0x11C */ u8          pendingLevelLoadId;   /* Copied to GameState.direct_level_load when levelLoadTimer reaches 0 */
    /* 0x11D */ u8          inputEnabled;         /* Enables PlatformEntityProcessInput from main platform tick */
    /* 0x11E */ u16         stateReturnTimer;     /* Countdown to EntityEnterAnimatedIdleState; trigger zones set 0x5A */
    /* 0x120 */ u16         levelLoadTimer;       /* Fade/level-load countdown; set to 0x20 by PlatformStateInit_FadeAndTimer */
    /* 0x122 */ u8          rgb[3];       /* Entity RGB tint (init 0x40,0x40,0x40) */
    /* 0x125 */ u8          _pad125[3];
} SoarPlayerEntity;  /* Size: 0x128 (296 bytes) */

/* -----------------------------------------------------------------------------
 * PathEnemyEntity (0x120 = 288 bytes; allocated 0x130)
 *
 * Enemy that follows predefined waypoint paths (entity types 076 and 084).
 * Created via EntityType076_PathEnemy_Init @ 0x8007F3DC ->
 * InitPathFollowingEnemy @ 0x8003C5B8. Waypoints come from the spawn
 * definition's variant field; if first==last waypoint the path loops
 * (loop flag set, count decremented). Frame-counter modulo staggers
 * multiple enemies sharing one path.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8       _pad100[4];
    /* 0x104 */ s32      eventStateMarker;   /* Event state FSM marker */
    /* 0x108 */ void    *eventStateCallback; /* Event state handler */
    /* 0x10C */ u8       _pad10C[4];
    /* 0x110 */ s16      pathOriginX;        /* Path origin X (base offset) */
    /* 0x112 */ s16      pathOriginY;        /* Path origin Y (base offset) */
    /* 0x114 */ void    *pWaypoints;         /* Waypoint array (s16 pairs: X, Y) */
    /* 0x118 */ u8       _pad118[4];
    /* 0x11C */ s16      waypointCount;      /* Number of waypoints in path */
    /* 0x11E */ u16      pathTimer;          /* Current path interpolation time */
} PathEnemyEntity;  /* Size: 0x120 (288 bytes) */

/* -----------------------------------------------------------------------------
 * Entity List Node
 * Used for tick/render/collision linked lists in GameState
 * ----------------------------------------------------------------------------- */
typedef struct EntityListNode {
    /* 0x00 */ struct EntityListNode *next;
    /* 0x04 */ Entity *entity;
} EntityListNode;

/* -----------------------------------------------------------------------------
 * Entity Callback System
 * 
 * Two-tier callback table architecture:
 * 
 * 1. EntityCallbackTableBase (0x20 bytes) - Used by simple entities
 *    Located at 0x8001044c-0x8001052c (8 pre-defined tables)
 *    Contains: destroy, tick, texture callbacks
 * 
 * 2. EntityCallbackTable (0x50+ bytes) - Used by complex entities  
 *    Per-entity-type tables (player @ 0x80011804, checkpoint @ 0x800111a8)
 *    Extends base with: state machine + input callbacks
 * 
 * Each callback slot is 8 bytes: [s16 entity_offset, s16 pad, func_ptr]
 * The entity_offset is added to the entity pointer before calling.
 * ----------------------------------------------------------------------------- */
typedef void (*EntitySlotCallback)(void *entity_adjusted);

/* Single callback slot (8 bytes) */
typedef struct {
    /* 0x00 */ s16 entity_offset;       /* Added to entity ptr before calling */
    /* 0x02 */ s16 pad;
    /* 0x04 */ EntitySlotCallback func; /* Callback function */
} EntityCallbackSlot;

/* -----------------------------------------------------------------------------
 * EntityCallbackTableBase (0x20 = 32 bytes)
 * 
 * Base callback table used by simple entities. 8 pre-defined instances
 * exist at 0x8001044c, 0x8001046c, 0x8001048c, 0x800104ac, 0x800104cc,
 * 0x800104ec, 0x8001050c, 0x8001052c.
 * 
 * Callbacks:
 *   destroy: DestroyEntityAndFreeMemory or similar
 *   tick:    UpdateEntityRender (0x8001d988)
 *   texture: UploadEntityTextureIfDirty (0x8001e5b8)
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32 unused_00;                   /* UNUSED - always 0, never accessed */
    /* 0x04 */ u32 unused_04;                   /* UNUSED - always 0, never accessed */
    /* 0x08 */ EntityCallbackSlot destroy;      /* Cleanup (called with param 3) */
    /* 0x10 */ EntityCallbackSlot tick;         /* Per-frame update */
    /* 0x18 */ EntityCallbackSlot texture;      /* Texture upload */
} EntityCallbackTableBase;  /* Size: 0x20 */

/* -----------------------------------------------------------------------------
 * BasicEntityVtable (0x18 = 24 bytes)
 *
 * Callback table for BasicEntity objects (sprite contexts, UI elements).
 * Same head layout as EntityCallbackTableBase but with only render +
 * cleanup slots. The two leading words are never accessed; they exist so
 * the slot offsets line up with the shared dispatch code.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32 unused_00;                    /* Always 0 (dispatch alignment) */
    /* 0x04 */ u32 unused_04;                    /* Always 0 */
    /* 0x08 */ EntityCallbackSlot render;        /* Render/draw callback */
    /* 0x10 */ EntityCallbackSlot freeResource;  /* Resource cleanup callback */
} BasicEntityVtable;  /* Size: 0x18 (24 bytes) */

/* -----------------------------------------------------------------------------
 * BasicEntityVtableShort (0x10 = 16 bytes)
 *
 * Minimal vtable with only a render callback (no cleanup needed).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32 unused_00;
    /* 0x04 */ u32 unused_04;
    /* 0x08 */ EntityCallbackSlot render;        /* Render callback only */
} BasicEntityVtableShort;  /* Size: 0x10 (16 bytes) */

/* -----------------------------------------------------------------------------
 * FSMStateSlot (8 bytes)
 *
 * A single tagged FSM state: [marker, handler] pair passed to
 * EntitySetState(entity, &slot.marker, slot.fn) and the other FSM
 * dispatchers. marker is almost always 0xFFFF0000 ("direct call,
 * offset 0" - see the marker encoding above). 184 such slots live in
 * .sdata (e.g. fsmSlot_PlayerStateCallback_2 @ 0x800A5D30); GameState
 * and Entity both begin with embedded slots of this shape.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ s32   marker;  /* FSM marker (see encoding above; usually 0xFFFF0000) */
    /* 0x04 */ void *fn;      /* State handler function pointer */
} FSMStateSlot;  /* Size: 0x08 */

/* -----------------------------------------------------------------------------
 * MenuCallbackTable (0x58 = 88 bytes)
 * 
 * Callback table for menu entities. Used by MenuInputHandler @ 0x80077af0.
 * Extends base with 8-byte EntityCallbackSlot entries for input handling.
 * 
 * Example: Checkpoint table at 0x800111a8
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00-0x1F: Base callbacks (same as EntityCallbackTableBase) */
    /* 0x00 */ u32 unused_00;
    /* 0x04 */ u32 unused_04;
    /* 0x08 */ EntityCallbackSlot destroy;
    /* 0x10 */ EntityCallbackSlot tick;
    /* 0x18 */ EntityCallbackSlot texture;
    
    /* 0x20-0x3F: Selection/action callbacks */
    /* 0x20 */ EntityCallbackSlot onSelect;     /* Item selected (navigation up/down) */
    /* 0x28 */ EntityCallbackSlot onDeselect;   /* Item deselected */
    /* 0x30 */ EntityCallbackSlot onConfirm;    /* X button (takes shoulder state param) */
    /* 0x38 */ EntityCallbackSlot onCancel;     /* Circle button */
    
    /* 0x40-0x57: Input callbacks */
    /* 0x40 */ EntityCallbackSlot onLeftRight;  /* Left/Right d-pad */
    /* 0x48 */ EntityCallbackSlot onShoulder;   /* L1/R1 shoulder buttons */
    /* 0x50 */ EntityCallbackSlot onDpad;       /* D-pad direction (0-7 enum) */
} MenuCallbackTable;  /* Size: 0x58 (88 bytes) */

/* -----------------------------------------------------------------------------
 * PlayerCallbackTable (0x80+ bytes)
 * 
 * Callback table for the player entity @ 0x80011804.
 * Base slots use 8-byte EntityCallbackSlot format, but 0x40+ is a DENSE
 * function pointer array (4 bytes each) for bounce/collision handlers.
 * 
 * The dense array at 0x40 contains 16 function pointers indexed by
 * collision/bounce type. Used by PlayerProcessBounceCollision.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00-0x1F: Base callbacks (same as EntityCallbackTableBase) */
    /* 0x00 */ u32 unused_00;
    /* 0x04 */ u32 unused_04;
    /* 0x08 */ EntityCallbackSlot destroy;      /* PlayerDestroy @ 0x80059b58 */
    /* 0x10 */ EntityCallbackSlot tick;         /* UpdateEntityRender */
    /* 0x18 */ EntityCallbackSlot texture;      /* UploadEntityTextureIfDirty */
    
    /* 0x20-0x3F: State callbacks (8-byte slots) */
    /* 0x20 */ EntityCallbackSlot state_20;     /* NULL for player */
    /* 0x28 */ EntityCallbackSlot state_28;     /* 0x8006ed54 */
    /* 0x30 */ EntityCallbackSlot state_30;     /* 0x8006ed4c */
    /* 0x38 */ EntityCallbackSlot state_38;     /* 0x8006ed44 */
    
    /* 0x40-0x7F: Dense function pointer array (4 bytes each, NOT slots!) */
    /* Used by PlayerProcessBounceCollision for indexed dispatch */
    /* 0x40 */ void *bounceHandlers[16];        /* 16 × 4 = 0x40 bytes */
} PlayerCallbackTable;  /* Size: 0x80 (128 bytes) */

/* Generic alias for code that doesn't know the specific table type */
typedef MenuCallbackTable EntityCallbackTable;

/* -----------------------------------------------------------------------------
 * EntityTypeCallback (8 bytes)
 *
 * Maps an entity type index to its init callback.
 * Table of 121 entries at 0x8009D5F8 (stored in GameState+0x7C).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32  state_flags;  /* Flags/state mask (usually 0x0000FFFF or 0xFFFFFFFF) */
    /* 0x04 */ void *callback;    /* InitEntity_XXXXXXXX function pointer */
} EntityTypeCallback;  /* Size: 0x08 */

/* -----------------------------------------------------------------------------
 * EntityDefinition (24 bytes)
 *
 * Spawn-time entity descriptor from Asset 501.
 * Raw 24-byte structs loaded into GameState+0x28 (entity_spawn_list).
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u16  x1;          /* Bounding box left (world tiles) */
    /* 0x02 */ u16  y1;          /* Bounding box top */
    /* 0x04 */ u16  x2;          /* Bounding box right */
    /* 0x06 */ u16  y2;          /* Bounding box bottom */
    /* 0x08 */ u16  xCenter;     /* Center X spawn position */
    /* 0x0A */ u16  yCenter;     /* Center Y spawn position */
    /* 0x0C */ u16  variant;     /* Entity variant / subtype parameter */
    /* 0x0E */ u32  padding0E;   /* Padding / unused */
    /* 0x12 */ u16  entityType;  /* Entity type index into EntityTypeCallback table */
    /* 0x14 */ u16  layer;       /* Layer assignment */
    /* 0x16 */ u16  padding16;   /* Padding */
} EntityDefinition;  /* Size: 0x18 (24 bytes) */

/* -----------------------------------------------------------------------------
 * CheatCode (16 bytes)
 *
 * 8-button sequence for cheat detection.
 * Table g_CheatCodeTable compared against GameState+0x17C circular buffer.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u16  btn[8];      /* Ordered button sequence (PSX button masks) */
} CheatCode;  /* Size: 0x10 (16 bytes) */

#endif /* ENTITY_H */
