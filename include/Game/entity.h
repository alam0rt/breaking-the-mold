#ifndef ENTITY_H
#define ENTITY_H

#include "common.h"

/* =============================================================================
 * ENTITY HIERARCHY
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
 * Extended types (Player 0x3E8, HUD 0x7530) build on Entity with extra fields.
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

struct Entity {
    /* State Machine - Tick System (0x00-0x0F) */
    /* 0x00 */ s32             tickMarker;       /* 0xFFFF0000 = direct call, else countdown */
    /* 0x04 */ EntityCallback  tickCallback;     /* Per-frame update function */
    /* 0x08 */ s32             eventMarker;      /* Event callback marker */
    /* 0x0C */ EntityCallback  eventCallback;    /* Event handler function */
    
    /* Entity Info (0x10-0x17) */
    /* 0x10 */ s16             allocSize;        /* Total allocation size for this entity */
    /* 0x12 */ s16             flags12;          /* Entity flags */
    /* 0x14 */ u8              active;           /* Active flag (init to 1) */
    /* 0x15 */ u8              pad15;            /* Padding (extended types may use) */
    /* 0x16 */ u8              pad16;            /* Padding (extended types may use) */
    /* 0x17 */ u8              pad17;            /* Padding (extended types may use) */
    
    /* Collision System (0x18-0x1F) */
    /* 0x18 */ void           *collisionVtable;  /* Collision handler vtable */
    /* 0x1C */ s32             renderMarker;     /* Render callback marker */
    
    /* Render Callback (0x20-0x23) */
    /* 0x20 */ EntityCallback  renderCallback;   /* Render function */
    
    /* Movement Callback X (0x24-0x2B) */
    /* 0x24 */ s16             moveMarkerX;      /* Movement marker X */
    /* 0x26 */ s16             moveCountX;       /* Movement count X */
    /* 0x28 */ s16             moveOffsetX;      /* Movement offset X */
    /* 0x2A */ u8              pad2A;            /* Padding */
    /* 0x2B */ u8              pad2B;            /* Padding */
    
    /* Movement Callback Y (0x2C-0x33) */
    /* 0x2C */ s16             moveMarkerY;      /* Movement marker Y */
    /* 0x2E */ s16             moveCountY;       /* Movement count Y */
    /* 0x30 */ s16             moveOffsetY;      /* Movement offset Y */
    /* 0x32 */ u8              pad32;            /* Padding */
    /* 0x33 */ u8              pad33;            /* Padding */
    
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
    /* 0x74 */ u8              facing;           /* Facing direction (0=left, 1=right) */
    /* 0x75 */ u8              flipY;            /* Vertical flip flag */
    /* 0x76 */ u8              stateFlags;       /* Entity state flags */
    /* 0x77 */ u8              boundsDirty;      /* Bounds need recalculation */
    
    /* Movement Callbacks (0x78-0x7F) */
    /* 0x78 */ EntityCallback  moveCallbackY;    /* Y movement callback */
    /* 0x7C */ EntityCallback  moveCallbackX;    /* X movement callback */
};  /* Size: 0x80 (128 bytes) */

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

#endif /* ENTITY_H */
