#ifndef ENTITY_H
#define ENTITY_H

#include "common.h"

/* -----------------------------------------------------------------------------
 * Entity (Base Structure)
 * 
 * Runtime base entity structure (0x80 = 128 bytes). All game entities share
 * this common layout. Extended entity types (Player, HUD, enemies, etc.)
 * allocate more memory and add type-specific fields after offset 0x78.
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
    
    /* Collision Bounds (0x50-0x53) */
    /* 0x50 */ s16             collisionX1;      /* Collision left (world coords) */
    /* 0x52 */ s16             collisionY1;      /* Collision top (init to 1) */
    
    /* Scale Factors - 16.16 Fixed Point (0x54-0x67) */
    /* 0x54 */ s32             scaleX;           /* X scale (0x10000 = 1.0) */
    /* 0x58 */ s32             scaleY;           /* Y scale (0x10000 = 1.0) */
    /* 0x5C */ s32             scale5C;          /* Scale factor 3 */
    /* 0x60 */ s32             scale60;          /* Scale factor 4 */
    /* 0x64 */ s32             scale64;          /* Scale factor 5 */
    
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
 * EntityCallbackTable
 * Callback table entry used by state machine system
 * ----------------------------------------------------------------------------- */
typedef void (*EntityTickFunc)(Entity *entity);

typedef struct {
    /* 0x00 */ u32 field_00;
    /* 0x04 */ u32 field_04;
    /* 0x08 */ u32 field_08;
    /* 0x0C */ u32 field_0C;
    /* 0x10 */ s16 entity_offset;      /* Offset to add to entity pointer */
    /* 0x12 */ s16 pad12;
    /* 0x14 */ EntityTickFunc tick;    /* Tick callback function */
    /* 0x18 */ u32 field_18;
} EntityCallbackTable;

#endif /* ENTITY_H */
