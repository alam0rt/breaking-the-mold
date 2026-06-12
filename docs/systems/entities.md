# Entity System

**Status**: ✅ Complete (all FUN_ functions named)  
**Last Updated**: January 20, 2026

Entities are game objects combining sprite graphics, behavior callbacks, and position/state data.

## Overview

The entity system has four key aspects:
1. **Entity data** - BLB Asset 501 stores placement (24-byte structures)
2. **Entity behavior** - Hardcoded init functions with sprite IDs
3. **Entity struct hierarchy** - Three tiers of runtime structures
4. **Variant parameter** - Per-entity modifier stored at offset +0x0C in placement data

**Critical**: Entity type → sprite ID mapping is **HARDCODED** in game code, not in BLB data.

**See also:**
- [Entity Types Reference](../reference/entity-types.md) - Callback table and variant system
- [Entity Identification Guide](entity-identification.md) - BLB→Internal type mapping
- [FSM / Callback Dispatch Patterns](fsm-callback-patterns.md) - Ghidra-backed marker+callback slot architecture

## Entity Composition Architecture (January 2026 Update)

This section documents the **composition patterns** used by different entity categories based on comprehensive Ghidra analysis.

### Base Entity Components

All entities share a common lifecycle managed through **vtables** (virtual method tables):

```
┌─────────────────────────────────────────────────────────────┐
│  ENTITY VTABLE STRUCTURE (EntityCallbackTable)              │
├─────────────────────────────────────────────────────────────┤
│  +0x00: unused_00        (always 0)                         │
│  +0x04: unused_04        (always 0)                         │
│  +0x08: destroy          (EntityCallbackSlot - cleanup)     │
│  +0x10: tick             (EntityCallbackSlot - per-frame)   │
│  +0x18: texture          (EntityCallbackSlot - VRAM upload) │
│  +0x20: onSelect         (menu only)                        │
│  ...                                                        │
└─────────────────────────────────────────────────────────────┘

EntityCallbackSlot (8 bytes):
  +0x00: s16 entity_offset  (offset into entity struct)
  +0x02: s16 pad
  +0x04: func_ptr           (callback function)
```

### Entity Lifecycle Functions

| Phase | Function | Address | Purpose |
|-------|----------|---------|---------|
| **Allocate** | `AllocateFromHeap` | 0x800143f0 | Get memory from BLB buffer |
| **Init Base** | `InitEntityStruct` | 0x8001a0c8 | Zero 128-byte base structure |
| **Init Full** | `InitFullEntityWithAnimation` | 0x8001c6c8 | Base + sprite context + animation |
| **Init Sprite** | `InitEntitySprite` | 0x8001c720 | Load sprite data, set z-order |
| **Register** | `AddEntityToSortedRenderList` | 0x800213a8 | Add to render list |
| **Register** | `AddToUpdateQueue` | - | Add to collision/update queue |
| **Destroy** | `DestroyEntity` | 0x80020974 | Full cleanup + optional free |
| **Destroy Base** | `DestroyEntityAndFreeMemory` | 0x8001ca60 | Free child entities + memory |

### Vtable Progression Pattern

Entities use **vtable progression** during destruction (C++ destructor chain emulation):

```c
void DestroyEntityAndFreeMemory(Entity* entity, uint flags) {
    // Vtable progresses through hierarchy during cleanup
    entity->vtable = 0x8001044c;  // Stage 1
    FreeChildResources(entity);
    entity->vtable = 0x8001046c;  // Stage 2  
    DestroyChildEntities(entity);
    entity->vtable = 0x800104ac;  // Stage 3 (base)
    if (flags & 1) FreeFromHeap(entity);
}
```

### Common Vtable Addresses

| Address | Category | Used By |
|---------|----------|---------|
| 0x8001039c | BasicEntity | UI helpers, overlays |
| 0x800104ac | MenuEntityBase | Menus, HUD |
| 0x8001044c | Full Entity | Animated entities |
| 0x8001046c | Entity (mid-destroy) | Destruction chain |
| 0x80010770 | Ground Enemy | Patrol enemies |
| 0x80010870 | Path-Following | Path-based entities |
| 0x80010da4 | Collectible | Items, pickups |
| 0x80010de4 | Path Enemy | Moving enemies |
| 0x80011288 | Boss (JoeHeadJoe) | Final boss |
| 0x800112a8 | Boss (ShrineyGuard) | Shriney boss |
| 0x80011308 | Boss (MonkeyMage) | Wizz boss |
| 0x80011328 | Boss (Klogg) | Multiple bosses |

---

## Entity Category Compositions

### 1. Base Entity (Simple Objects)

**Size**: 16 bytes (BasicEntity) or 128 bytes (Entity)  
**Init Function**: `InitBasicEntityWithVtable` @ 0x8001543c

```c
void InitBasicEntity(Entity* entity, u16 size) {
    entity->vtable = 0x8001039c;
    entity->allocSize = size;
    entity->active = 1;
    // Zero positions, no sprite/animation
}
```

**Components**:
- ✅ Position (screen X/Y)
- ✅ Active flag
- ✅ Vtable pointer
- ❌ No sprite
- ❌ No animation
- ❌ No collision

**Used For**: UI helpers, colored overlays, simple effects.

---

### 2. Standard Entity (Animated Objects)

**Size**: 128 bytes base + type-specific extensions  
**Init Chain**: `InitEntityStruct` → `InitFullEntityWithAnimation` → `InitEntitySprite`

```c
void InitStandardEntity(Entity* entity, u32 spriteId, s16 z, s16 x, s16 y) {
    InitEntityStruct(entity, ALLOC_SIZE);
    entity->vtable = 0x8001044c;
    ClearSpriteContextWrapper(entity + 0x78);  // Sprite context
    InitEntityAnimationState(entity);           // Animation state
    SetEntitySprite(entity, spriteId, z, x, y);
}
```

**Components**:
- ✅ Position (world X/Y)
- ✅ Velocity (vX/vY)
- ✅ Sprite context
- ✅ Animation state
- ✅ Render bounds
- ✅ Hitbox
- ✅ Tick callback
- ✅ Event callback

**Used For**: Decorations, particles, simple collectibles.

---

### 3. Collectible Entity (Pickups)

**Size**: ~0x130 bytes  
**Init Chain**: `InitEntityWithSprite` → `InitCollectibleEntity`

```c
void InitCollectibleEntity(Entity* entity, EntityDef* def, s16 z) {
    entity->allocSize = 900;
    entity->entityDef = def;
    entity->tickCallback = CollectibleTickCallback;
    entity->eventCallback = EntityEventHandlerWalk;
    entity->collisionMask = 4;  // Player collision
    SetupEntityScaleCallbacks(entity);
    UpdateCollectibleTriggerZone(entity, def, z);
}
```

**Additional Components**:
- ✅ Entity definition reference (+0x100/0x40)
- ✅ Collision mask (type 4 = player contact)
- ✅ Scale callbacks (shrink/grow powerups)
- ✅ Trigger zone bounds

**Callback Pattern**:
```
Tick:  CollectibleTickCallback @ 0x8003acc8
Event: EntityEventHandlerWalk @ (handles walk messages)
```

**Used For**: Clayballs, ammo, items, health.

---

### 4. Ground Patrol Enemy

**Size**: 0x124 bytes (main) + 0x120 bytes (child)  
**Init Function**: `InitGroundPatrolEnemy` @ 0x8002ea3c

```c
Entity* InitGroundPatrolEnemy(Entity* entity, EntityDef* def) {
    // 1. Init main entity with sprite
    InitEntitySprite(entity, 0x8c510186, 0x3de, def->x, def->y, 0);
    entity->vtable = 0x80010770;
    
    // 2. Setup path/decor system
    InitPathFollowingDecorEntity(entity, def, 0);
    
    // 3. Set tick callback
    entity->tickMarker = 0xFFFF0000;
    entity->tickCallback = CollectiblePhartHeadTickCallback;  // Misnamed
    
    // 4. Configure semi-transparency for sprite
    poly = entity->spriteContext;
    poly->r = 0x20; poly->g = 0x60; poly->b = 0x30;
    
    // 5. CREATE CHILD ENTITY (shadow/effect)
    Entity* child = AllocateFromHeap(0x120);
    InitEntitySprite(child, 0xa9240484, 0x3de, def->x, def->y, 0);
    InitPathFollowingDecorEntity(child, def, 0);
    entity->childEntity = child;  // Store at +0x120
    child->worldX = entity->worldX - 8;
    child->worldY = entity->worldY - 16;
    child->tickCallback = EntityUpdateCallback;
    
    AddEntityToSortedRenderList(g_GameStatePtr, child);
    return entity;
}
```

**Unique Components**:
- ✅ Child entity at +0x120 (shadow/effect)
- ✅ Path-following system
- ✅ Semi-transparent rendering
- ✅ Texture page configuration

**Callback Pattern**:
```
Tick:   CollectiblePhartHeadTickCallback @ 0x8002ec3c (patrol behavior)
Child:  EntityUpdateCallback @ 0x8001cb88 (basic animation)
Vtable: 0x80010870 (path-following) → 0x80010770 (ground enemy)
```

---

### 5. Path-Following Enemy

**Size**: ~0x130 bytes  
**Init Function**: `InitPathFollowingEnemy` @ 0x8003c5b8

```c
Entity* InitPathFollowingEnemy(Entity* entity, EntityDef* def, u32* spriteTable,
                                u8 param4, u8 param5, u8 param6) {
    // 1. Init with sprite from table
    InitEntityWithSprite(entity, spriteTable, 0x3ca, def->x, def->y - 1);
    entity->vtable = 0x80010da4;
    
    // 2. Setup collectible base (shared logic)
    InitCollectibleEntity(entity, def, 0x3ca);
    
    // 3. Sound by variant type
    short variant = *(entity->entityDef + 0x12);
    switch(variant) {
        case 10:  PlayEntityPositionSound(entity, 0x7c108242); break;
        case 0x1a: PlayEntityPositionSound(entity, 0x7c108244); break;
        case 0x1b: PlayEntityPositionSound(entity, 0x68009240); break;
        case 0x54: PlayEntityPositionSound(entity, 0x42c11270); break;
        case 0x60: PlayEntityPositionSound(entity, 0xc2820c70); break;
    }
    
    // 4. PATH DATA SETUP
    entity->tickCallback = SoundEmitterTickCallback;
    entity->spriteTable = spriteTable;
    
    u16 pathId = *(entity->entityDef + 0xc);  // Variant field
    if (pathId != 0) {
        entity->eventCallback = EntityFollowPathWithWrapping;
        GetEntitySpawnData(g_GameStatePtr, pathId, &entity->pathData, &entity->pathCount);
        // Initialize position along path...
    }
    return entity;
}
```

**Unique Components**:
- ✅ Path data system (GetEntitySpawnData lookup)
- ✅ Position sound (variant-dependent)
- ✅ Sprite table reference (multiple sprites)
- ✅ Path wrapping at endpoints

**Callback Pattern**:
```
Tick:  SoundEmitterTickCallback @ 0x8003c950 (with panning)
Event: EntityFollowPathWithWrapping @ 0x80055c70
```

---

### 6. Boss Entity Architecture

Bosses are the most complex entities, featuring:
- Multiple child entities (HP bar, projectiles, platforms)
- State machine with attack patterns
- Custom event handlers
- HP tracking via `g_pPlayerState[0x1D]`

#### Boss Common Pattern

```c
void InitBoss_Common(Entity* boss, EntityDef* def, u32* spriteTable) {
    // 1. Base initialization via collectible (handles position)
    CreateCollectibleFromSpawn(boss, def, spriteTable);
    
    // 2. Set boss-specific vtable
    boss->vtable = BOSS_VTABLE;  // e.g., 0x80011308
    
    // 3. Initialize player HP (stored in PlayerState!)
    g_pPlayerState[0x1d] = BOSS_HP;  // 3-5 depending on boss
    
    // 4. Create HP bar HUD entity
    Entity* hpBar = AllocateFromHeap(0x104);
    InitEntitySprite(hpBar, HP_BAR_SPRITE, 10000, 0xa2, 0xf4, 0);
    hpBar->tickCallback = BossHPBarTickCallback;
    hpBar->hpValue = g_pPlayerState[0x1d];
    AddEntityToSortedRenderList(g_GameStatePtr, hpBar);
    
    // 5. Set event handler for damage
    boss->eventCallback = BossEventHandler;
    
    // 6. Set initial state
    EntitySetState(boss, INITIAL_STATE_MARKER, InitialStateCallback);
}
```

#### Known Boss Implementations

| Boss | Level | HP | Init Function | Vtable | Notes |
|------|-------|----|--------------|---------| ------|
| **Klogg** | Multiple | 5 | `InitKloggBossEntity` @ 0x80047288 | 0x80011328 | Uses `g_KloggBossSprites` |
| **MonkeyMage** | WIZZ (21) | 5 | `InitMonkeyMageBoss` @ 0x80047fb8 | 0x80011308 | 6 rotating platforms + force field |
| **GlennYntis** | - | - | `InitGlennYntisBoss` @ 0x80049a54 | - | - |
| **ShrineyGuard** | SHRI (9) | 3 | `InitShrineyGuardBoss` @ 0x8004af64 | 0x800112a8 | Lower HP, ground-based |
| **JoeHeadJoe** | HEAD (15) | 5 | `InitJoeHeadJoeBoss` @ 0x8004c0e0 | 0x80011288 | Brick-breaker mechanics |

#### Boss Event Handler

```c
int BossEventHandler(Entity* boss, u16 event, int param3, int param4) {
    switch(event) {
        case 0x1001:  // Ball hit (upper)
        case 0x1002:  // Ball hit (lower)
            if (boss->vulnerableFlag) {
                g_pPlayerState[0x1d]--;  // Decrement HP
                if (g_pPlayerState[0x1d] == 0) {
                    EntitySetState(boss, DEATH_STATE);
                } else {
                    EntitySetState(boss, ATTACK_LOOP_STATE);
                }
            }
            break;
            
        case 0x0001:  // Animation marker
            // Spawn effects based on param3
            break;
            
        case 0x1008:  // Register projectile
            boss->activeProjectile = param4;
            break;
    }
}
```

#### MonkeyMage Boss Composition (Most Complex)

```
MonkeyMage Boss Entity (0x200+ bytes)
├── Main boss body (sprite from g_MonkeyMageBossSprites)
├── HP Bar entity (sprite 0x181c3854)
├── 6x Rotating Platform entities (InitCircularPlatformEntity)
│   └── Positions from table at 0x8009b860
├── Force Field entity (vtable 0x80011328)
│   └── Color = vulnerability state (red=invuln, blue=vuln)
└── Additional boss part (vtable 0x80011348)
    └── Sparkle/effect entity
```

---

### 7. Destructor Patterns

Entity destructors follow a consistent pattern with vtable progression:

#### Simple Destructor (Type 0-6)

```c
void EntityDestructor_TypeN(Entity* entity, uint flags) {
    entity->vtable = 0x800105cc;  // Mark destruction in progress
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(blbHeaderBufferBase, entity, 0, 0);
    }
}
```

#### Destructor with Child Cleanup

```c
void EntityDestructor_DestroyAllChildEntities(Entity* entity, uint flags) {
    entity->vtable = 0x800104cc;
    RemoveFromUpdateQueue(entity);
    RemoveFromZOrderList(entity);
    ClearEntityDefList(entity);
    FreeEntityLists(entity);
    
    if (entity->spriteContext != NULL) {
        __builtin_delete(entity->spriteContext);
    }
    ConditionalDelete(entity + 0x84, 2);
    
    entity->vtable = 0x800104ec;
    if (flags & 1) {
        FreeFromHeap(blbHeaderBufferBase, entity, 0, 0);
    }
}
```

#### Destructor with SPU Cleanup (Sound Entities)

```c
void SoundEmitterDestroyCallback(Entity* entity, uint flags) {
    if (entity->spuVoiceIndex >= 0) {
        SpuSetKey(0, 1 << entity->spuVoiceIndex);  // Stop voice
    }
    EntityDestructor_Simple(entity, flags);
}
```

---

### Entity Size Reference

| Entity Type | Allocation Size | Description |
|-------------|-----------------|-------------|
| BasicEntity | 0x10 (16) | Minimal UI helper |
| MenuEntityBase | 0x1C (28) | Menu/HUD base |
| Entity (base) | 0x80 (128) | Standard entity |
| Collectible | 0x100-0x130 | Items, pickups |
| Ground Enemy | 0x124 (main) + 0x120 (child) | Patrol enemies |
| Path Enemy | 0x130 | Moving enemies |
| Boss | 0x200+ | Boss entities |
| HP Bar | 0x104 | Boss HP display |
| Player | 0x3E8 (1000) | Player entity |
| HUD | 0x7530 (30000) | Full HUD system |

---

## Enemy Entity State Machines

Enemies use a state machine pattern driven by the tick callback and event handler.

### Ground Enemy States

```
┌──────────────┐     Timer/Event     ┌──────────────┐
│    IDLE      │ ─────────────────>  │    PATROL    │
│ (wait/spawn) │                     │  (walk L/R)  │
└──────────────┘                     └──────────────┘
       │                                    │
       │  Player Near                       │ Edge/Wall
       v                                    v
┌──────────────┐                     ┌──────────────┐
│    CHASE     │                     │ TURN_AROUND  │
│ (pursue)     │                     │ (flip facing)│
└──────────────┘                     └──────────────┘
       │                                    │
       │  Hit by Player                     │
       v                                    v
┌──────────────┐                     ┌──────────────┐
│    DEATH     │ <────────────────── │   FALLING   │
│  (particles) │                     │ (off ledge)  │
└──────────────┘                     └──────────────┘
```

### Enemy State Functions

| State | Function | Address | Behavior |
|-------|----------|---------|----------|
| Patrol | `EnemyPatrolState` | 0x8003dd20 | Walk in direction, check edges |
| Falling | `InitEnemyFallingState` | 0x8003de08 | Apply gravity, check ground |
| Death | `EnemyDeathState` | 0x8003defc | Spawn particles, destroy |
| Idle Timer | `EnemyIdleTimerState` | 0x8004e9f4 | Wait for timer, then activate |
| Sprite | `EnemySpriteState` | 0x8004eac8 | Animation-only state |
| Defeat | `EnemyDefeatState` | 0x8004eb94 | Post-death cleanup |

### Enemy Tick Patterns

```c
// EnemyTickWithCollision @ 0x8004839c
void EnemyTickWithCollision(Entity* enemy) {
    TickEntityAnimation(enemy);
    
    // Check player collision
    if (CheckEntityBoxCollision(enemy, PLAYER_MASK)) {
        // Player hit enemy - send damage event
        SendEventToEntity(enemy, 0x1001, 0, g_PlayerEntity);
    }
    
    // Check offscreen
    if (IsEntityOffScreen(enemy)) {
        DestroyEntity(enemy);
    }
}

// EnemyUpdateWithCollisionAndDeath @ 0x8004b2ec
void EnemyUpdateWithCollisionAndDeath(Entity* enemy) {
    EnemyTickWithCollision(enemy);
    
    if (enemy->hp <= 0) {
        EnemyDeathWithParticles(enemy);
    }
}
```

### Path-Following Enemy Movement

```c
// EntityFollowPathWithWrapping @ 0x80055c70
void EntityFollowPathWithWrapping(Entity* enemy) {
    short* pathData = enemy->pathData;
    short pathLen = enemy->pathCount;
    short pathIndex = enemy->pathIndex;
    
    // Get current and next waypoints
    short x1 = pathData[pathIndex * 2];
    short y1 = pathData[pathIndex * 2 + 1];
    short x2 = pathData[(pathIndex + 1) * 2];
    short y2 = pathData[(pathIndex + 1) * 2 + 1];
    
    // Interpolate position
    enemy->worldX = Lerp(x1, x2, enemy->pathProgress);
    enemy->worldY = Lerp(y1, y2, enemy->pathProgress);
    
    // Advance along path
    enemy->pathProgress += enemy->speed;
    if (enemy->pathProgress >= 0x100) {
        enemy->pathProgress = 0;
        enemy->pathIndex++;
        
        // Wraparound at end
        if (enemy->pathIndex >= pathLen) {
            if (enemy->pathLooping) {
                enemy->pathIndex = 0;
            } else {
                // Reverse direction
                enemy->speed = -enemy->speed;
            }
        }
    }
    
    // Face movement direction
    enemy->facing = (x2 > x1) ? 1 : 0;
}
```

---

## Boss Attack State Machines

Bosses use complex state machines with attack patterns, invulnerability windows, and phase transitions.

### Boss State Flow (Generic)

```
┌─────────────────┐
│   SPAWN/INTRO   │  ←── Initial animation, invulnerable
└────────┬────────┘
         │
         v
┌─────────────────┐     ┌─────────────────┐
│      IDLE       │ ──> │   ATTACK_PREP   │  ←── Wind-up animation
└────────┬────────┘     └────────┬────────┘
         │                       │
         │                       v
         │              ┌─────────────────┐
         │              │    ATTACKING    │  ←── Spawn projectiles/hazards
         │              └────────┬────────┘
         │                       │
         │  HP > 0               │  HP > 0
         └───────────────────────┘
                   │
                   │  HP == 0
                   v
         ┌─────────────────┐
         │     DEATH       │  ←── Death animation, victory trigger
         └─────────────────┘
```

### Boss-Specific State Tables

**Klogg Boss States** (from 0x800a5b60):
| Marker | Callback | State |
|--------|----------|-------|
| 0xFFFF0000 | 0x800479d0 | Intro/Idle |
| 0xFFFF0000 | BossRandomAttackChoice | Attack Selection |
| 0xFFFF0000 | ShrineyGuardStartLoopAttackState | Loop Attack |
| 0xFFFF0000 | ShrineyGuardDeathState | Death |

**JoeHeadJoe Boss** (Brick-Breaker Style):
- Attack pattern table at 0x8009bb28
- Ball types: 0=Normal (catchable), 1=Spiky (hazard), 2=Special
- Lower HP = more spiky balls in pattern

### Boss HP System

Boss HP is stored in `g_pPlayerState[0x1D]` (NOT the boss entity):

```c
// HP initialization varies by boss
InitKloggBoss:        g_pPlayerState[0x1d] = 5;  // 5 hits
InitShrineyGuardBoss: g_pPlayerState[0x1d] = 3;  // 3 hits
InitMonkeyMageBoss:   g_pPlayerState[0x1d] = 5;  // 5 hits
InitJoeHeadJoeBoss:   g_pPlayerState[0x1d] = 5;  // 5 hits
```

### Boss Vulnerability System

Bosses track vulnerability at entity offset +0x115:

```c
// In BossEventHandler
case 0x1001:  // Hit event
case 0x1002:
    if (boss->vulnerableFlag) {  // +0x115 != 0
        g_pPlayerState[0x1d]--;
        // State transition
    }
    break;
```

Vulnerability windows are controlled by:
1. State machine (invulnerable during intro/attack animations)
2. Entity properties (force field color for MonkeyMage)
3. Animation markers (specific frames can toggle vulnerability)

---

> **See Also**: [Entity Types Reference](../reference/entity-types.md) for full callback table (121 entries) and type mappings.

### Entity Subsystems

| Subsystem | Functions | Description |
|-----------|-----------|-------------|
| VRAM Slot Entities | 4 | Entities with dedicated VRAM texture slots |
| Multi-Part Entities | 3 | Composite entities with 5 sub-entity array |
| Path-Following Entities | 33 | Entities that move along predefined paths |
| Projectile System | 24 | Homing missiles, bouncing projectiles |
| Death/Particle System | 30 | Death animations, debris spawning |
| Sound Entities | 41 | Entities with SPU voice management |
| Menu System | 47 | UI elements, buttons, cursors |
| Timer Entities | 37 | Countdown-based state transitions |

## Entity Structure Hierarchy

**Verified in Ghidra 2026-01-19** - Three-tier entity system:

### BasicEntity (16 bytes)
**Init**: `InitBasicEntityWithVtable` @ 0x8001543c  
**Usage**: Simple UI helpers, overlays

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| 0x00 | s16 | screenX | Screen X position |
| 0x02 | s16 | screenY | Screen Y position |
| 0x04 | s16 | widthOrU | Width or VRAM U coord |
| 0x06 | s16 | heightOrV | Height or VRAM V coord |
| 0x08 | s16 | zOrder | Render z-order (lower = behind) |
| 0x0A | u8 | active | Enable flag (1=active) |
| 0x0B | u8 | pad | Padding |
| 0x0C | ptr | vtable | Callback vtable (0x8001039c) |

### MenuEntityBase (28 bytes)
**Init**: `InitMenuEntityWithVtable` @ 0x80019748  
**Usage**: Menu/UI elements, HUD components

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| 0x00 | s32 | tickMarker | FSM marker (0xFFFF0000 = direct call) |
| 0x04 | ptr | tickCallback | Per-frame update function |
| 0x08 | s32 | eventMarker | Event callback marker |
| 0x0C | ptr | eventCallback | Event handler function |
| 0x10 | s16 | zOrder | Render z-order |
| 0x12 | s16 | reserved | Unused |
| 0x14 | u8 | active | Enable flag |
| 0x15-0x17 | u8[3] | pad | Padding |
| 0x18 | ptr | vtable | Callback vtable (0x800104ac) |

### Entity (128 bytes)
**Init**: `InitEntityStruct` @ 0x8001a0c8  
**Usage**: Gameplay entities, enemies, items, player

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| 0x00 | s32 | tickMarker | State machine tick marker |
| 0x04 | ptr | tickCallback | Per-frame update callback |
| 0x08 | s32 | eventMarker | Event callback marker |
| 0x0C | ptr | eventCallback | Event handler |
| 0x10 | s16 | allocSize | Total allocation size |
| 0x12 | s16 | collisionMask | Collision layer mask |
| 0x14 | u8 | active | Active flag |
| 0x18 | ptr | collisionVtable | Collision handler vtable |
| 0x1C | s32 | renderMarker | Render callback marker |
| 0x20 | ptr | renderCallback | Render function |
| 0x24-0x33 | - | movement | Movement markers and offsets |
| 0x34 | ptr | spriteContext | Sprite render context |
| 0x38-0x3F | s16[4] | renderBox | Render offset X/Y, width/height |
| 0x40-0x47 | s16[4] | hitbox | Hitbox offset X/Y, width/height |
| 0x48-0x4F | s16[4] | screenBox | Screen bounds (calculated) |
| 0x50-0x5F | s32[4] | scale | Render/powerup/parallax scales |
| 0x68 | s16 | worldX | World X position |
| 0x6A | s16 | worldY | World Y position |
| 0x6C | s16 | velocityX | X velocity |
| 0x6E | s16 | velocityY | Y velocity |
| 0x70 | s16 | targetX | Target X for movement |
| 0x72 | s16 | targetY | Target Y for movement |
| 0x74 | u8 | facing | 0=left, 1=right |
| 0x75 | u8 | flipY | Vertical flip |
| 0x76 | u8 | textureDirty | Needs VRAM upload |
| 0x77 | u8 | boundsValid | Screen bounds calculated |
| 0x78 | ptr | moveCallbackY | Y movement callback |
| 0x7C | ptr | moveCallbackX | X movement callback |

### Extended Entities

Player and HUD entities extend the 128-byte base:

| Entity Type | Alloc Size | Extended Fields |
|-------------|------------|-----------------|
| Player | 0x3E8 (1000) | PlayerState at +0x80, animation at +0xA0+ |
| HUD | 0x7530 (30000) | HUD data buffer, UI state |
| Boss | ~0x200+ | Boss-specific state machines |

## Game Loop Integration

Entities are managed through a frame-based game loop executed in `main` @ 0x800828b0.
a
### Mode System Architecture

**Important**: There is **only ONE mode callback** (`GameModeCallback @ 0x8007e654`), not multiple mode-specific callbacks. The "mode" concept refers to three different systems:

1. **BLB Content Mode** (header+0xF36): Determines which data structures load via `GetCurrentModeReservedData @ 0x8007ae9c`
   - Mode 1: Movie entries (28 bytes @ header+0xB64)
   - Mode 2: Credits entries (12 bytes @ header+0xF1C)
   - Mode 3: Level entries (112 bytes @ header+0x56)
   - Mode 4/5: Demo entries (16 bytes @ header+0xCD3)
   - Mode 0, 6: No reserved data

2. **Level Loading Mode** (param_2): Controls level execution behavior passed to `SetupAndStartLevel`
   - param_2=1: Normal gameplay (live controller input)
   - param_2=5: Demo Mode 1 (input replay from buffer)
   - param_2=6: Demo Mode 2 (alternate demo replay)
   - param_2=99: Menu trigger mode

3. **Audio Mode** (0x800A6082): Used by `PlaySoundEffect @ 0x8007c388` to adjust sound behavior
   - Set via `SetGameMode @ 0x8007c36c` (validates 0-6)
   - Cleared to 0 by `UploadAudioToSPU @ 0x8007c088`

**All modes execute through the same callback** - they differ in which data loads and whether input is live or replayed, but the execution flow is identical.

> **See Also**: [Demo/Attract Mode System](demo-attract-mode.md) for input replay details.

### Frame Processing Flow

```
1. TickCDStreamBuffer()          - Stream CD data (every 4 frames)
2. PadRead(1)                    - Read controller input
3. UpdateInputState()            - Process P1/P2 input
4. Mode Callback                 - Level-specific logic
   └─> GameModeCallback @ 0x8007e654
       ├─ Pause/menu handling
       ├─ Level loading/respawn
       ├─ Checkpoint save/restore
       ├─ FUN_80081d0c()              - Spawn entities (alternate system)
       ├─ SpawnOnScreenEntities()     - Spawn from Asset 501
       └─ EntityTickLoop()            - Update entity callbacks
5. WaitForVBlankIfNeeded()       - VSync if needed
6. RenderEntities()              - Draw all entities
7. DrawSync(0)                   - Wait for GPU
8. Layer Render Callback         - Draw tile layers
9. DrawSync(0)                   - Wait for GPU
10. VSync(2) if needed           - Frame timing
11. ProcessDebugMenuInput()      - Debug menu
12. FlushDebugFontAndEndFrame()  - Present frame
```

### Game Mode Callback @ 0x8007e654

The mode callback coordinates level state, spawning, and entity processing:

```c
void GameModeCallback(GameState* state) {
    // Pause counter handling
    if (state[0x160] != 0) {
        state[0x160]--;
        // Fade out if countdown hits 1
        return;
    }
    
    // Menu/pause input handling
    // START button detection, fade callbacks
    
    // Level transition logic
    if (state[0x146] || state[0x147] || state[0x148]) {
        SetupAndStartLevel(...);
    }
    
    // Respawn handling
    if (state[0x146] && !state[0x19c]) {
        if (g_pPlayerState[0x11] == 0) {
            // Level complete - advance
            FUN_8007a578(state + 0x84);
            SetupAndStartLevel(state, 99);
        } else {
            RespawnAfterDeath(state);
        }
    }
    
    // Checkpoint system
    if (state[0x149] && !state[0x14a]) {
        SaveCheckpointState(state);      // Save entities to +0x134
    }
    if (state[0x14a]) {
        RestoreCheckpointEntities(state); // Restore from +0x134
    }
    
    // Entity spawning (only if not paused/loading)
    if (!state[0x150] && !state[0x14a]) {
        FUN_80081d0c(state);             // Spawn from alternate system
        SpawnOnScreenEntities(state);    // Spawn from Asset 501
    }
    
    // Entity processing (only if not paused)
    if (!state[0x190]) {
        EntityTickLoop(state);           // Update all entities
        UpdateCameraPosition(state);     // Camera scroll
    }
}
```

### Entity Processing Loop @ 0x80020b34

Iterates entity tick list and calls callbacks:

```c
void EntityTickLoop(GameState* state) {
    Entity* entity = *(state + 0x1C);  // Tick list head
    
    while (entity != NULL) {
        // Render layer management at z_order threshold
        if (entity->z_order > 1999 && !rendered) {
            FUN_800233c0(state);  // Camera scroll update
            rendered = true;
        }
        
        // Call entity update callback
        if (entity->callback_main != NULL) {
            (*entity->callback_main)(entity);
        }
        
        // Deferred entity removal
        FUN_80020c74(state);
        
        entity = entity->next;
    }
    
    state[0x10C]++;  // Increment frame counter
}
```

### Entity Removal @ 0x80020c74

Handles deferred entity destruction:

```c
void DeferredEntityRemoval(GameState* state) {
    if (state[0x34] != 0) {  // Entity marked for removal
        if (state[0x38] == 0) {
            RemoveEntityFromAllLists(state[0x34]);
            state[0x34] = 0;
        } else {
            RemoveFromUpdateQueue(state);
            if (state[0x38] != 1) {
                RemoveFromRenderList(state);
            }
            RemoveFromTickList(state, state[0x34]);
            
            // Call entity destructor
            Entity* entity = state[0x34];
            if (entity != NULL) {
                void* vtable = entity[0x18];
                code* destructor = *(vtable + 0xC);
                short offset = *(vtable + 8);
                (*destructor)(entity + offset, 3);  // Cleanup mode
            }
            state[0x34] = 0;
        }
        state[0x38] = 0;
    }
}
```

### Camera Update @ 0x800233c0

Complex camera scroll logic called during entity processing:

```c
void UpdateCameraPosition(GameState* state) {
    Entity* player = state[0x30];
    if (player == NULL || state[99] != 0) return;
    
    // Calculate target camera position from player
    // Applies screen offsets, level bounds clamping
    // Uses lookup tables at 0x8009b074, 0x8009b104, 0x8009b0bc
    // for smooth scrolling acceleration curves
    
    // Update camera velocity (state+0x4c, state+0x50)
    // Apply velocity with sub-pixel precision
    // Clamp to level bounds from tile header
    
    state[0x44] = clamped_x;  // Camera X
    state[0x46] = clamped_y;  // Camera Y
}
```

### Rendering Pipeline @ 0x80020e80

Renders all entities and handles background color updates:

```c
void RenderEntities(GameState* state) {
    // Update background color if requested
    if (state[0x130] != 0) {
        u8 r = state[0x131];
        u8 g = state[0x132];
        u8 b = state[0x133];
        
        // Double-buffered write
        blbHeaderBufferBase[0x1d] = r;    // FB1 red
        blbHeaderBufferBase[0x1e] = g;    // FB1 green
        blbHeaderBufferBase[0x1f] = b;    // FB1 blue
        blbHeaderBufferBase[0x505d] = r;  // FB2 red
        blbHeaderBufferBase[0x505e] = g;  // FB2 green
        blbHeaderBufferBase[0x505f] = b;  // FB2 blue
        
        state[0x130] = 0;  // Clear flag
    }
    
    // Empty iteration over tick list (+0x1C) - optimization artifact?
    
    // Render loop over z-sorted render list (+0x20)
    for (node = state[0x20]; node != NULL; node = node->next) {
        Entity* entity = node[1];
        void* vtable = entity[0xC];
        code* render_func = *(vtable + 0xC);
        short offset = *(vtable + 8);
        
        (*render_func)(entity + offset);
    }
}
```

**Background Color Format:**
- Offset 0x1d/5e/5f: RGB bytes (0-255)
- Two copies for double-buffering (0x5040 byte stride)
- Updated when state[0x130] flag set (collision events, triggers)

**Render List:**
- Z-sorted during insertion (AddEntityToSortedRenderList @ 0x800213a8)
- Each entity has method table (vtable) at entity+0xC
- Render callback at vtable+0xC with adjusted pointer (entity+offset)

## Complete Game Loop Reference

Full main loop with all discovered functions:

| Order | Function | Address | Purpose |
|-------|----------|---------|---------|
| 1 | TickCDStreamBuffer | 0x8007ccb8 | CD streaming (every 4 frames) |
| 2 | PadRead | PSY-Q | Read controller ports |
| 3 | UpdateInputState | 0x800259d4 | Process P1/P2 input |
| 4 | GameModeCallback | 0x8007e654 | Level coordinator |
| 4a | └─ SpawnEntitiesAlternateSystem | 0x80081d0c | Spawn from 128-byte array |
| 4b | └─ SpawnOnScreenEntities | 0x80024288 | Spawn from Asset 501 |
| 4c | └─ EntityTickLoop | 0x80020b34 | Update entity callbacks |
| 5 | WaitForVBlankIfNeeded | 0x8001352c | Conditional VSync |
| 6 | RenderEntities | 0x80020e80 | Entity rendering |
| 7 | DrawSync | PSY-Q | Wait for GPU |
| 8 | [Layer Render Callback] | via GameState+0xC | Tile layer rendering |
| 9 | DrawSync | PSY-Q | Wait for GPU again |
| 10 | VSync timing | PSY-Q | Optional 2-frame wait |
| 11 | ProcessDebugMenuInput | 0x80082c10 | Debug level select |
| 12 | FlushDebugFontAndEndFrame | 0x80013500 | Finalize frame |

**Frame Timing Modes:**
- Normal: VSync locks to 60 FPS (NTSC) or 50 FPS (PAL)
- Flag 0x06 set: Wait 2 vblanks (30 FPS mode)
- g_SkipVSync: Run unlocked (benchmarking/loading)

**CD Streaming:**
- ProcessCDStreamState @ 0x80038ef0 handles async CD reads
- State machine with retry logic (max 6 retries)
- Sector table at 0x8009b3d8, CdlLOC params at 0x8009b43c
- Enables audio/FMV streaming without blocking gameplay

## Asset 501 - Entity Placement Data

24-byte structures loaded from tertiary segment.

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    x1 (bbox left, pixels)
0x02    2     u16    y1 (bbox top, pixels)
0x04    2     u16    x2 (bbox right, pixels)
0x06    2     u16    y2 (bbox bottom, pixels)
0x08    2     u16    x_center (spawn position)
0x0A    2     u16    y_center (spawn position)
0x0C    2     u16    variant (animation/subtype)
0x0E    4     u32    padding (always 0)
0x12    2     u16    entity_type
0x14    2     u16    layer (with flags)
0x16    2     u16    padding
```

### Entity Count

Stored in Asset 100 (Tile Header) at offset 0x1E.

Accessor: `GetEntityCount` @ 0x8007b7a8

### Layer Field (offset 0x14)

```
Bits 0-7:  Render layer (1, 2, or 3)
Bits 8-15: Render flags (purpose unverified)
```

Most entities use simple values (1, 2, 3). Some use extended values like 0xF301.

**IMPORTANT**: This field does NOT determine z_order! Entity z_order is hardcoded per entity type.

## Known Entity Types

Common entity types observed in BLB data:

| Type | Name | Description |
|------|------|-------------|
| 2 | Clayball | Collectible coins (5,727 total) |
| 3 | Ammo | Standard bullet pickup |
| 8 | Item | Generic item pickup |
| 24 | SpecialAmmo | Special ammunition |
| 25, 27 | EnemyA/B | Enemy entities |
| 28, 48 | PlatformA/B | Moving platforms |
| 42 | Portal | Warp point |
| 45 | Message | Save/message box |
| 50, 51 | Boss/BossPart | Boss entities |
| 60, 61 | Particle/Sparkle | Visual effects |

For the complete list of 121 entity types with callback addresses, see [Entity Types Reference](../reference/entity-types.md).

## Runtime Entity Structure

**Note**: This section documents the **OLD** understanding. See "Entity Structure Hierarchy"
at the top of this document for the **VERIFIED** Ghidra struct definitions.

The base Entity struct is **128 bytes** (0x80), not 0x44C. Extended entities allocate more:
- Player: 0x3E8 bytes (1000)
- HUD: 0x7530 bytes (30000)
- Standard enemies: 0x100-0x200 bytes typically

### Extended Entity Fields (Player, beyond +0x80)

Fields beyond the 128-byte base are entity-type-specific:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x80+ | ... | PlayerState | 30-byte PlayerState embedded |
| 0xA0+ | ... | animation | Animation state machine |
| 0xB4 | 4 | frame_x_scale | X scale (16.16 fixed-point) |
| 0xB8 | 4 | frame_y_scale | Y scale (16.16 fixed-point) |
| 0xDA | 2 | current_frame | Current animation frame index |
| 0xDE | 2 | target_frame | Target animation frame |
| 0xE0 | 2 | pending_flags | Pending state change flags |
| 0xEC | 2 | frame_rate_divisor | CORRECTED: frame duration divisor (countdown is at 0xEE) - see UpdateEntityRender @ 0x8001D988 |
| 0xF0-0xF2 | 3 | anim_state | CORRECTED x2: current anim direction/loop/active bytes (ApplyPendingSpriteState @ 0x8001D554) - NOT rgb; 0xF3-0xF5 are the pending equivalents |
| 0xF6 | 1 | visibility | Rendering flag |
| 0x1AF | 1 | cheat_effect_1 | Cheat code effect flag (from cheat 0x11) |
| 0x1B0 | 1 | cheat_effect_2 | Cheat code effect flag (from cheat 0x14) |
| 0x1B1 | 1 | cheat_effect_3 | Cheat code effect flag (from cheat 0x15) |

## Entity Lifecycle

```
1. ALLOCATION
   AllocateFromHeap(blbHeaderBuffer, size, 1, 0) @ 0x800143f0
   └── Allocates from BLB buffer (16-byte aligned blocks)
   └── Size varies: 0x80 (base), 0x3E8 (player), 0x7530 (HUD)

2. INITIALIZATION
   InitEntityWithSprite(entity, sprite_id, z_order, x, y, flags) @ 0x8001c868
   ├── InitEntityStruct(entity, size) @ 0x8001a0c8 - Zero/init 128-byte base
   ├── FUN_8007bbc0() - GPU/render state setup
   ├── FUN_8001954c() - Animation state init
   ├── FUN_8001c980() - Entity core setup
   ├── FUN_8001cea4() - Load sprite data
   └── FUN_8001d080() - Finalize setup

3. CALLBACK ASSIGNMENT
   entity->tickCallback = EntityUpdateCallback;  // Default @ 0x8001cb88
   entity->eventCallback = event_handler;
   entity->renderCallback = render_func;

4. REGISTRATION
   AddEntityToSortedRenderList(GameState, entity) @ 0x800213a8 - Z-order list
   AddToZOrderList(GameState, entity) @ 0x80020f68 - Z-sorted at +0x1C
   AddToXPositionList(GameState, entity) @ 0x8002107c - X-sorted at +0x20

5. TICK LOOP
   EntityTickLoop (0x80020e1c)
   └── For each entity in tick_list_head:
       └── entity->tickCallback(entity)
           ├── TickEntityAnimation() @ 0x8001d290
           ├── ApplyPendingSpriteState() @ 0x8001d554
           └── UpdateSpriteFrameData() @ 0x8001d748

6. DESTRUCTION
   MarkEntityForDeferredRemoval @ 0x80020D74
   └── Sets GameState+0x34 = entity, +0x38 = mode
   DeferredEntityRemoval @ 0x80020B1C
   └── Removes from all lists, calls destructor via vtable

6. STATE TRANSITIONS
   EntitySetState(entity, param1, param2) @ 0x8001eaac
   └── Dispatches to state-specific callbacks

7. DESTRUCTION
   FreeFromHeap(blbHeaderBuffer, ptr, size, flags) @ 0x800145a4
```

## Entity Init Functions

91 functions call `InitEntitySprite` with hardcoded parameters:

```c
// Example signatures:
InitEntitySprite(entity, 0x21842018, 10000, x, y, 0);  // Player
InitEntitySprite(entity, 0x168254b5, 959, x, y, 1);    // Particles
InitEntitySprite(entity, 0xa89d0ad0, 1001, x, y, 0);   // Entity
```

### Known z_order Values

| Entity | z_order | Purpose |
|--------|---------|---------|
| Player | 10000 | Front of most layers |
| UI/HUD | 10000 | Always visible |
| Particles | 959 | Effects behind gameplay |
| General | ~1000 | Gameplay layer |

## Entity Loader (LoadEntitiesFromAsset501 @ 0x80024dc4)

Loads 24-byte entity definitions from Asset 501 into a linked list at `GameState+0x28`.

**This does NOT spawn entities** - it only loads the placement data. Actual entity spawning
happens later via `SpawnOnScreenEntities` when entities come into view.

```c
entity_count = GetEntityCount(ctx);  // From Asset 100 +0x1E
entity_data = ctx[14];  // Asset 501 pointer

// Copy each 24-byte entity def into a linked list node
for (i = 0; i < entity_count; i++) {
    EntityDef* def = AllocateFromHeap(blbHeaderBufferBase, 24, 1, 0);
    memcpy(def, entity_data + i * 24, 24);
    
    // Add to linked list at GameState+0x28
    ListNode* node = AllocateFromHeap(blbHeaderBufferBase, 8, 1, 0);
    node->next = GameState->entityDefList;
    node->data = def;
    GameState->entityDefList = node;
}
```

## Entity Spawn Dispatcher (SpawnOnScreenEntities @ 0x80024288)

**THIS IS THE SINGLE HOOK POINT FOR ALL ENTITY SPAWNING!**

Called every frame from the game mode callback (`FUN_8007e654`). Iterates the entity definition
list at `GameState+0x28` and spawns any entities that come on screen.

### Algorithm

```c
void SpawnOnScreenEntities(GameState* state) {
    // Iterate entity definition list
    for (ListNode* node = state->entityDefList; node != NULL; node = node->next) {
        EntityDef* def = node->data;
        
        // Check if entity is on screen (+/- 16px margin)
        if (!IsEntityOnScreen(state, def)) continue;
        
        // Check if already spawned (bit 0 of def->flags at +0x16)
        if (def->flags & 0x01) continue;
        
        // Lookup callback from table
        EntityTypeEntry* entry = state->callbackTable + (def->entity_type * 8);
        EntityCallback callback = entry->callback;
        
        // CALL THE CALLBACK - this allocates and initializes the entity
        callback(state, def);
        
        // Mark as spawned
        def->flags |= 0x01;
    }
}
```

### EntityDef Structure (24 bytes)

```c
struct EntityDef {                    // Offset  Type   Description
    u16 x1;                           // +0x00   u16    Bounding box left
    u16 y1;                           // +0x02   u16    Bounding box top
    u16 x2;                           // +0x04   u16    Bounding box right
    u16 y2;                           // +0x06   u16    Bounding box bottom
    u16 x_center;                     // +0x08   u16    Spawn X position
    u16 y_center;                     // +0x0A   u16    Spawn Y position
    u16 variant;                      // +0x0C   u16    Animation/subtype
    u32 padding;                      // +0x0E   u32    Always 0
    u16 entity_type;                  // +0x12   u16    Type (indexes callback table)
    u16 layer;                        // +0x14   u16    Render layer + flags
    u16 flags;                        // +0x16   u16    Bit 0: spawned flag
};
```

### Callback Table Lookup

The callback table is at `GameState+0x7C` (points to `0x8009D5F8` with 121 entries):

```c
struct EntityTypeEntry {              // 8 bytes per entry
    u32 flags;                        // +0x00   Usually 0xFFFF0000
    void (*callback)(GameState*, EntityDef*);  // +0x04   Function pointer
};

// Lookup:
EntityTypeEntry* entry = *(GameState + 0x7C) + (entity_type * 8);
callback = entry->callback;
```

### Callback Signature

```c
void EntityTypeXXX_Callback(GameState* state, EntityDef* def) {
    // Allocate entity structure (size varies by type)
    Entity* entity = AllocateFromHeap(blbHeaderBufferBase, SIZE, 1, 0);
    
    // Initialize entity with sprite ID (hardcoded per type)
    InitEntitySprite(entity, SPRITE_ID, z_order, def->x_center, def->y_center, flags);
    
    // Set entity-specific callbacks
    entity->callback_main = EntityUpdateCallback;
    
    // Add to game lists
    AddEntityToSortedRenderList(state, entity);
    AddToUpdateQueue(state, entity);
}
```

### Hook Point for Runtime Tracing

To capture entity spawning in `game_watcher.lua`, add a breakpoint at **0x80024288**
or hook the callback invocation to record:

- `entity_type` (entityDef+0x12)
- `x_center` (entityDef+0x08)
- `y_center` (entityDef+0x0A)
- `variant` (entityDef+0x0C)
- `layer` (entityDef+0x14)
- `callback` address being invoked
- Current frame number

This captures ALL entity spawning with complete type information, allowing cross-reference
with player interactions (pickups, collisions, etc.).

## Entity Tick Loop (EntityTickLoop @ 0x80020e1c)

Called each frame from main loop:

```c
void EntityTickLoop(GameState* state) {
    Entity* entity = state->entity_list;  // +0x1C
    
    while (entity != NULL) {
        if (entity->callback_main != NULL) {
            entity->callback_main(entity);
        }
        entity = entity->next;
    }
}
```

## Key Functions

### Core Entity Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `InitEntityStruct` | 0x8001a0c8 | Zero and init 0x44C byte structure |
| `InitEntityWithSprite` | 0x8001c868 | Full entity+sprite initialization |
| `InitEntitySprite` | 0x8001c720 | Core entity sprite init |
| `EntityTickLoop` | 0x80020e1c | Main update loop |
| `EntityUpdateCallback` | 0x8001cb88 | Default entity tick callback |
| `EntitySetState` | 0x8001eaac | State machine transitions |

### Animation System

| Function | Address | Purpose |
|----------|---------|---------|
| `TickEntityAnimation` | 0x8001d290 | Animation frame tick handler |
| `ApplyPendingSpriteState` | 0x8001d554 | Apply pending sprite changes |
| `UpdateSpriteFrameData` | 0x8001d748 | Update frame dimensions/offsets |

### Entity Lists

| Function | Address | Purpose |
|----------|---------|---------|
| `AddEntityToSortedRenderList` | 0x800213a8 | Sorted render list insertion |
| `AddToZOrderList` | 0x80020f68 | Z-order sorted list (+0x1C) |
| `AddToXPositionList` | 0x8002107c | X-position sorted list (+0x20) |
| `AddPreInitEntitiesToList` | 0x800250c8 | Pre-init entities from palette data |

### Memory Management

| Function | Address | Purpose |
|----------|---------|---------|
| `AllocateFromHeap` | 0x800143f0 | Block-based heap allocator |
| `FreeFromHeap` | 0x800145a4 | Free allocated memory |

### Entity Loading

| Function | Address | Purpose |
|----------|---------|---------|
| `LoadEntitiesFromAsset501` | 0x80024dc4 | Load entity defs to linked list |
| `SpawnOnScreenEntities` | 0x80024288 | **SPAWN DISPATCHER** - calls callbacks from table |
| `GetEntityCount` | 0x8007b7a8 | Entity count accessor |
| `GetEntityDataPtr` | 0x8007b7bc | Asset 501 data pointer |

### Player/Boss Creation

| Function | Address | Purpose |
|----------|---------|---------|
| `SpawnPlayerAndEntities` | 0x8007df38 | Player creation dispatcher |
| `CreatePlayerEntity` | 0x800596a4 | Default player creation |
| `CreateCameraEntity` | 0x80044f7c | Camera entity creation |
| `InitPlayerEntity` | 0x8001fcf0 | Player setup |
| `InitBossEntity` | 0x80047fb8 | Boss setup |
| `InitPlayerSpriteAvailability` | 0x80059a70 | Check 7 player sprites |

## VRAM Slot Entity System

**Address Range**: 0x800318e0 - 0x80031da0

Entities that allocate dedicated VRAM texture slots for dynamic rendering (e.g., color-keyed overlays).

| Function | Address | Purpose |
|----------|---------|--------|
| `InitVRAMSlotEntity` | 0x800318e0 | Allocate 8x16 VRAM slot, setup vtable 0x80010b68 |
| `DestroyVRAMSlotEntity` | 0x80031984 | Free VRAM slot if allocated |
| `RenderVRAMSlotOverlay` | 0x80031a14 | Complex SPRT/TILE_1/DR_OFFSET/DR_AREA rendering |
| `CheckVRAMSlotPixelColor` | 0x80031da0 | StoreImage + check pixel == 0x3c0f (magenta marker) |

**Pattern**: These entities use StoreImage to read back GPU pixels and check for specific color values.

## Multi-Part Entity System

**Address Range**: 0x80032124 - 0x80032800

Composite entities with an array of 5 sub-entities at offset +0x104. Used for segmented enemies/bosses.

| Function | Address | Purpose |
|----------|---------|--------|
| `MultiPartEntityTick` | 0x80032124 | Iterate 5 sub-entities, send 0x1010/0x100f/0x1009 messages |
| `MultiPartEntityRenderTick` | 0x80032454 | UploadEntityTextureIfDirty, VRAM color check |
| `MultiPartEntityMessageHandler` | 0x80032800 | Handle 0x100f/0x1009/0x1010 flag operations |

**Message Codes**:
- `0x1010` - Set flag
- `0x100f` - Clear flag  
- `0x1009` - Toggle flag

## Path-Following Entity System

**Address Range**: 0x80032e0c - 0x80033xxx, 0x8003c5b8+, 0x80055790+

Entities that move along predefined path data. Used for platforms, enemies, and decorations.

### Core Path Functions

| Function | Address | Purpose |
|----------|---------|--------|
| `InitPathFollowEntity` | 0x80032e0c | GetEntitySpawnData x2, alloc 3 buffers (0x34, 0x34, 2) |
| `DestroyPathFollowEntity` | 0x80032f3c | Free path buffers |
| `RenderPathEntitySegments` | 0x80032fec | POLY_GT4 with depth bucket, distance coloring |
| `InitPathFollowEntityAlt` | 0x800335d8 | Alternate path init |
| `CalculatePathDistance` | 0x80055790 | Distance calculation along path |
| `UpdateEntityAlongPath` | 0x800558b8 | Apply velocity along path |
| `UpdateEntityPathWithWrapping` | 0x80055c70 | Path with wraparound at endpoints |

### Path Animation Tables

| Address | Size | Purpose |
|---------|------|--------|
| `0x8009bc08` | 200 entries | Primary path animation data |
| `0x8009bf28` | 61 entries | Secondary path data |

### Entity Types Using Paths

| Function | Address | Entity Type |
|----------|---------|-------------|
| `EntityType027_PathEnemy_Init` | 0x8007f354 | Type 27 - Path enemy |
| `EntityType076_PathEnemy_Init` | 0x8007f3dc | Type 76 - Alt path enemy |
| `EntityType070_PathDecor_Init` | 0x800807f8 | Type 70 - Path decoration |
| `EntityType082_PathHazard_Init` | 0x8008127c | Type 82 - Path hazard |

## Entity Data Locations

### LevelDataContext (GameState+0x84)

| Offset | Field | Description |
|--------|-------|-------------|
| +0x38 (ctx[14]) | entityData | Asset 501 pointer (24-byte entity definitions) |

### GameState Entity Lists

| Offset | Field | Description |
|--------|-------|-------------|
| +0x1C | tickListHead | Entity tick list (z-sorted, iterated by EntityTickLoop) |
| +0x20 | renderListHead | Entity render list (z-sorted, iterated by RenderEntities) |
| +0x24 | updateQueueHead | Collision/update queue list |
| +0x28 | entityDefListHead | Entity definition pool (raw defs from Asset 501) |
| +0x2C | playerEntityAlt | Player entity (alternate reference) |
| +0x30 | playerEntity | Main player entity pointer |

## Entity Type Callback Table

The game uses a static callback table at `0x8009d5f8` to dispatch entity initialization/behavior
by type. This table is populated during `RemapEntityTypesForLevel` and stored at `GameState+0x7c`.

**Table Structure**: 121 entries (types 0-0x78), 8 bytes each:
```c
struct EntityTypeEntry {
    u32 flags;         // State flags (often 0xFFFF0000)
    void* callback;    // Init/tick callback function pointer
};
```

**Address**: `g_EntityTypeCallbackTable` @ `0x8009d5f8`

**Example entries** (from ROM):
| Type | Flags | Callback | Description |
|------|-------|----------|-------------|
| 0 | 0xFFFF0000 | 0x8007efd0 | Default/unused |
| 1 | 0xFFFF0000 | 0x8007f730 | Type 1 handler |
| 2 | 0xFFFF0000 | 0x80080328 | Type 2 handler |
| ... | ... | ... | ... |

**Loading flow**:
1. `RemapEntityTypesForLevel` @ 0x8008150c converts BLB entity types → internal types (0-0x78)
2. Internal type indexes into the callback table
3. Callback initializes entity behavior

## Entity Collision System

Collision detection is handled via the entity update queue at `GameState+0x24`.

### Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `CheckEntityCollision` | 0x800226f8 | Main collision check |
| `CollisionCheckWrapper` | 0x8001b47c | Collision check wrapper |
| `CheckBBoxOverlap` | 0x8001b3f0 | Bounding box overlap test |

### Collision Flow

```
1. Entity tick calls CollisionCheckWrapper(entity, type_mask, message, data)
2. CollisionCheckWrapper wraps CheckEntityCollision with entity's bbox
3. CheckEntityCollision:
   - type_mask == 2: Fast path - check player at GameState+0x2c directly
   - Other: Iterate GameState+0x24 queue for matching entities
4. If collision: Invoke target entity's state callback with message
5. Caller can check return value to determine if collision occurred
```

### Special Case: Clayballs (type_mask = 2)

Clayballs use an optimized collision path:
- Instead of iterating the collision queue, directly check the player entity
- Player entity stored at `GameState+0x2c`
- On collision, GameState callback receives message `3` (COLLECTED)

> **See Also**: [Entity Types Reference - Clayball Collision System](../reference/entity-types.md#clayball-collision-system) for detailed flow.

## Sprite ID Lookup

Entity type → sprite ID is hardcoded in init functions. The BLB does NOT contain this mapping.

To add a new entity type → sprite mapping, you must:
1. Find the init function in Ghidra
2. Extract the sprite ID constant
3. Verify the sprite exists in the level's tertiary container

## Common Entity Callback Patterns

**Verified via Ghidra analysis 2026-01-25** - Systematic review of all entity callbacks revealed these patterns:

### Tick Callback Patterns

All tick callbacks follow `void callback(Entity* entity, void* param2, short param3)` signature.

| Pattern | Example Function | Description |
|---------|-----------------|-------------|
| **Basic Tick** | `EntityUpdateCallback` @ 0x8001cb88 | Animation update only |
| **Tick + Collision** | `EntityUpdateWithCollisionWrapper` @ 0x80019d54 | Tick + collision check |
| **Tick + Timer** | `TimedEntityTickCallback` @ 0x80043464 | Decrement +0x104, trigger on 0 |
| **Tick + Path** | `EntityPathMovementUpdate` @ 0x80042ffc | Path interpolation |
| **Tick + Parent Cleanup** | `EntityOffscreenParentCleanupTick` @ 0x80045750 | Clean up if parent offscreen |

**Timer Locations**: Entity timers at +0x104, +0x110, +0x112, +0x80 (8-bit decrements)

### Event Handler Patterns

Event handlers use `int callback(Entity* entity, short event, short param, int source)` signature.

| Event Code | Name | Description |
|------------|------|-------------|
| 0x1001 | HIT | Entity was hit/attacked |
| 0x1002 | RELEASE | Contact released |
| 0x1008 | ATTACH | Attached to something |
| 0x1009 | TRIGGER | Trigger activated |
| 0x1017 | TELEPORT_CHECK | Teleporter availability check |
| 2 | CONTACT | General contact event |
| 3 | COLLECTED | Item collected |

**Event Handler Variants**:
- `EntityEventPassthrough_Event2` @ 0x80042fd0 - Only passes event 2
- `HazardEventHandler_0x1001` @ 0x80042f3c - Transitions on hit
- `EntityEventHandlerWalkWithTimer` @ 0x80046420 - Walk + timer decrement

### Init/State Callback Patterns

State init functions set up vtables and sprites:

```c
void InitSomethingState(Entity* entity) {
    entity[0] = 0xFFFF0000;           // Tick marker
    entity[1] = TickCallback;         // Tick function
    entity[2] = 0xFFFF0000;           // Event marker  
    entity[3] = EventHandler;         // Event function
    SetEntitySpriteId(entity, SPRITE_ID, 1);
    entity[0x26] = 0xFFFF0000;        // Animation end marker
    entity[0x27] = NextStateCallback; // Animation complete callback
}
```

### Destroy Callback Pattern

All destroy callbacks follow this pattern:

```c
void EntityDestroyCallback_0xXXXXXXXX(Entity* entity, uint flags) {
    entity[0x18] = 0xXXXXXXXX;       // VTable address (type identifier)
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(blbHeaderBufferBase, entity, 0, 0);
    }
}
```

**VTable Addresses by Entity Category**:
| Range | Category |
|-------|----------|
| 0x80011048-0x800111c8 | Sound entities (stop SPU voice first) |
| 0x80011068-0x80011188 | Standard game entities |
| 0x800111e8-0x80011248 | Enemy/path-following entities |
| 0x80011328-0x80011368 | Background/particle entities |

### Entity Type Systems

**Platform/Hazard System** (0x8004239c - 0x80043750):
- `InitPlatformEntityState` - Timer-based activation
- `PlatformTimerTickCallback` - Wait for timer or event
- `HazardTimerTickCallback` - Countdown to active state

**Teleporter System** (0x8004453c - 0x80045314):
- `TeleporterTickCallback` - Scale based on player distance
- `TeleporterActivateTickCallback` - Entry animation with fade
- `TeleporterTransitionTickCallback` - Wobble effect during transport

**Collectible System** (0x80045fac - 0x800465e0):
- `CollectibleScaledTickCallback` - Visibility based on game mode
- `InitItemRevealState` - Sparkle reveal animation

**Sound Emitter System** (0x8004591c - 0x8004598c):
- `SoundEmitterDestroyCallback` - Stop SPU voice on destroy
- `SoundEmitterWithPanningTick` - Update panning based on position

**Boss System** (0x80047288+):
- `InitKloggBossEntity` - Klogg boss setup with sprites from g_KloggBossSprites
- `KloggEventHandlerWithTrigger` - Multi-hit event handling
- `InitKloggChaseState` - Chase state with child entity management

**Menu System** (0x8007621c - 0x80076ad8):
- `MenuEntityDestroyCallback` @ 0x80019864 - Frees CLUT slot + heap memory
- `OptionsMenuDestroyCallback` @ 0x80076ad8 - Stage-specific cleanup (background/audio)
- `MenuLogoAnimEventHandler` @ 0x8007683c - Cycles through g_MenuLogoSprites on event
- `HUDDigitDisplayTickCallback` @ 0x8007811c - Displays digit (value % 10), hide if zero

**Password Entry System** (0x80076300 - 0x8007662c):
- `PasswordDigitSelectActive` @ 0x8007621c - Shows cursor on current slot
- `PasswordDigitSelectInactive` @ 0x80076300 - Hides cursor, shows digit
- `PasswordDigitInputHandler` @ 0x8007662c - Stores digit at +0x140 buffer, advances
- `PasswordBackspaceHandler` @ 0x8007650c - Decrements slot index
- `PasswordSubmitEventHandler` @ 0x80076378 - DecodePassword → SeekToLevelInSequence on success

**Ending/Credits Sequence** (0x800798b0 - 0x80079c24):
State machine flow: Reveal → Delay → Scroll → Scroll2 → Complete → FadeOut
- `EndingCreditsRevealTick` @ 0x800798b0 - Reveals credit entries one at a time
- `EndingCreditsDelayTick` @ 0x80079984 - 60-frame pause, plays sound on complete
- `EndingCreditsScrollTick` @ 0x80079a08 - Decrements +0x136 counter, increments scroll Y
- `EndingCreditsScrollTick2` @ 0x80079a90 - Decrements +0x142 counter
- `EndingCreditsCompleteTick` @ 0x80079b18 - Stops SPU voice, transition to fadeout or idle
- `EndingCreditsFadeOutTick` @ 0x80079c24 - Applies easing curve from DAT_8009cbbc

### Tilemap/Parallax Rendering Functions

These are NOT entity callbacks but core rendering infrastructure:

| Address | Name | Description |
|---------|------|-------------|
| 0x8001652c | `RenderTilemapLayerWithScroll` | Renders tilemap layer with H/V scroll |
| 0x80017af8 | `RenderTilemapPrimitivesWithBounds` | Bounds-check + DR_OFFSET + AddPrim |
| 0x800181fc | `RenderTilemapWithWrapAround` | Handles level wrap-around at edges |
| 0x800188e0 | `SubmitPrimitiveBufferToGPU` | Iterates prim buffer, submits by type |
| 0x8001a33c | `WorldToScreenX` | Simple: worldX + GameState+0x44 |
| 0x8001a358 | `WorldToScreenYWithParallax` | Fixed-point 16.16 parallax scale |
| 0x8001ab14 | `UpdateParallaxLayerPosition` | Complex parallax calc with callbacks |
| 0x8001bec0 | `IsPositionOffscreenLeft` | Returns true if screenX < -16 |

### CLUT/Palette Animation Functions

Menu/visual effects use animated color lookup tables:

| Address | Name | Description |
|---------|------|-------------|
| 0x8001991c | `CLUTPaletteCycleTickCallback` | Rotates palette entries via memmove |
| 0x80019a14 | `CLUTColorLerpTickCallback` | Interpolates RGB 5:5:5 per channel |

**CLUT Animation Entity Fields**:
- +0x1c, +0x20: Source/destination color buffers
- +0x30: Frame index (0-based)
- +0x39: Timer decrement (8-bit)
- +0x48+: Target VRAM rect for UploadTextureOrClut

## Related Documentation

- [Entity Types Reference](../reference/entity-types.md) - Full callback table (121 entries)
- [Game Functions Reference](../reference/game-functions.md) - Function addresses
- [Game Loop](game-loop.md) - Main loop and player creation
- [Player Animation](player-animation.md) - Player sprite system details
- [Sprites](sprites.md) - Sprite data format
- [Rendering Order](rendering-order.md) - Entity z_order
- [BLB Asset Handling](../blb-asset-handling.md) - Asset 501 details
- [Level Loading](level-loading.md) - Entity loading flow
