---
title: "Boss Entity Pattern"
category: systems
tags: [systems, boss, entity, pattern]
---

# Boss Entity Pattern

This document describes the common architectural pattern used by all boss entities in Skullmonkeys (SLES-01090).

## Overview

Boss entities are complex multi-entity systems that share a common pattern:
1. **Main Boss Entity** - The primary boss with state machine, HP tracking, and attack patterns
2. **HP Bar Entity** - HUD element showing remaining boss HP
3. **Child Entities** - Boss-specific projectiles, platforms, or effects

## Boss Entity List

| Type | Name | Level | HP | Init Function | Address |
|------|------|-------|----|----|---------|
| 71 | MonkeyMage | WIZZ (21) | 5 | `InitMonkeyMageBoss` | 0x80047fb8 |
| 100 | GlennYntis | GLEN (3) | 5 | `InitGlennYntisBoss` | 0x80049a54 |
| 101 | ShrineyGuard | SHRI (9) | 3 | `InitShrineyGuardBoss` | 0x8004af64 |
| 102 | JoeHeadJoe | HEAD (15) | 5 | `InitJoeHeadJoeBoss` | 0x8004c0e0 |
| 103 | Klogg | KLOG (24) | 5 | `InitKloggBoss` | 0x8004d88c |

## Common Entity Structure

All boss entities follow the same memory layout for critical fields:

```c
// Entity callback offsets (dwords)
entity[0-1]   = tick callback pointer
entity[2-3]   = event handler pointer  
entity[6]     = vtable pointer
entity[7-8]   = position callback (animation offsets)
entity[0x26-0x27] = animation completion callback

// Boss-specific offsets (bytes)
entity+0x74   = facing direction (0=left, 1=right)
entity+0x106  = hit flag (boss has been hit)
entity+0x107  = death sequence flag
entity+0x114  = attack type (ball/projectile variant)
entity+0x115  = attack pattern index / vulnerable flag
entity+0x116  = active state flag
entity+0x117  = timer/state flag
entity+0x40*4 = spawn definition pointer
entity+0x44*4 = facing from spawn data

// GameState boss defeat flags
g_GameStatePtr+0x19c = boss_defeated (1 when boss HP = 0)
g_GameStatePtr+0x19d = boss_facing (for cutscene direction)
```

## HP System

Boss HP is stored in player state, not in the boss entity:
- **Location**: `g_pPlayerState[0x1D]`
- **Starting HP**: Set in boss Init function (3 for ShrineyGuard, 5 for others)
- **Decremented by**: Event handlers (0x1001 or 0x1002 events)
- **Death trigger**: When HP reaches 0, transition to death state

## HP Bar Entity Pattern

All bosses create an HP bar HUD entity with this pattern:

```c
// Allocate HP bar entity
puVar = AllocateFromHeap(blbHeaderBufferBase, 0x104, 1, 0);

// Initialize with boss-specific sprite
InitEntitySprite(puVar, HP_SPRITE_ID, Z_ORDER, X_POS, Y_POS, 0);

// Set vtable for HP bar type
puVar[6] = 0x800112e8;

// Set tick callback to update sprite based on HP
puVar[0] = 0xffff0000;
puVar[1] = BossHPBarTickCallback;  // 0x8004992c

// Set position callbacks (usually fixed screen position)
puVar[9] = 0xffff0000;
puVar[10] = GetWorldPositionX;
puVar[0xb] = 0xffff0000;
puVar[0xc] = GetWorldPositionY;

// Store initial HP for comparison
*(byte*)(puVar + 0x40) = g_pPlayerState[0x1d];

// Add to render list
AddEntityToSortedRenderList(g_GameStatePtr, puVar);
```

### HP Bar Sprites

| Boss | Sprite ID | Screen Position |
|------|-----------|-----------------|
| JoeHeadJoe | 0x6101db0 | (0xA2, 0xF4) |
| Klogg | 0xcaadc032 | (0xA2, 0xF4) |
| GlennYntis | 0x48d81135 | (0xA2, 0xF4) |
| ShrineyGuard | 0xa8482860 | (0xA2, 0xF0) |
| MonkeyMage | 0x181c3854 | HUD style |

### BossHPBarTickCallback (0x8004992c)

The HP bar tick callback:
1. Reads current HP from `g_pPlayerState[0x1D]`
2. Compares against stored HP at `entity+0x100`
3. If HP decreased, updates sprite animation frame
4. If HP = 0, sends event 3 to GameState (triggers boss defeat sequence)

## Boss Event Handler Pattern

All bosses use event handlers for damage and animations:

### Common Events

| Event | Purpose |
|-------|---------|
| 0x0001 | Animation marker (param_3 = marker ID) |
| 0x1001 | Ball/projectile hit boss (upper body) |
| 0x1002 | Ball/projectile hit boss (lower body) |
| 0x1008 | Register ball/projectile with boss |
| 0x1009 | Ball tracking update |

### Animation Markers (Event 0x0001)

| Marker | Action |
|--------|--------|
| 0x400b43a0 | Spawn wind-up effect |
| 0x18443181 | Set invincibility timer (0x14 frames) |
| 0x46384180 | Trigger victory sequence (fade + END movie) |
| 0x684d3884 | Spawn ball projectile (JoeHeadJoe) |

## Boss State Machines

### Common State Pattern

```
Init → IdleState → (attack/player proximity) → AttackState → (animation complete) → IdleState
                 → (HP = 0) → DeathState → (animation complete) → Victory
```

### State Transition Functions

Each boss has state setter functions that:
1. Clear/set flags (vulnerable, active, hit)
2. Set tick callback for new state
3. Set event handler for new state
4. Set sprite ID for new animation
5. Set animation completion callback
6. Play state transition sound

### Death State Pattern

All death states follow this pattern:
```c
void BossDeathState(entity) {
    // Set boss defeated flag
    g_GameStatePtr[0x19c] = 1;
    g_GameStatePtr[0x19d] = entity+0x44 (facing);
    
    // Clear vulnerable flag
    entity[0x115] = 0;
    
    // Set death callbacks
    entity[0-1] = death tick callback;
    entity[2-3] = death event handler;
    
    // Set death animation sprite
    SetEntitySpriteId(entity, DEATH_SPRITE, 1);
    
    // Set animation completion callback for victory transition
    entity[0x26-0x27] = victory callback;
}
```

## Boss-Specific Details

### JoeHeadJoe (Brick Breaker)

**Mechanic**: Breakout/brick-breaker style - player controls paddle (pot), balls damage boss

**Ball Types**:
| Type | Value | Init Function | Behavior |
|------|-------|---------------|----------|
| Regular | 0 | `InitJoeHeadJoeBallRegular` (0x80053afc) | Catchable, reflects off paddle |
| Spiky | 1 | `InitJoeHeadJoeBallSpiky` (0x80054060) | HAZARD - damages player! |
| Special | 2 | `InitJoeHeadJoeBallSpecial` (0x800543c0) | Scaled/animated variant |

**Attack Pattern Table** (0x8009bb28):
- 5 phases based on (5 - HP): Phase 0 = full HP, Phase 4 = 1 HP
- 18 entries per phase, cycles 0-17
- Lower HP = more spiky balls in pattern

### Klogg (Final Boss)

**Mechanic**: Butt-bounce to damage when vulnerable

**Phases**: Uses `EnemyIdleTimerState` for pacing between attacks

**Special**: Creates chase sequence before main boss fight (`InitKloggChaseState`)

### ShrineyGuard

**Mechanic**: Butt-bounce during vulnerable window

**States**:
- `ShrineyGuardIdleState` - Waiting
- `ShrineyGuardAttackAnimState` - Wind-up
- `ShrineyGuardSetAttackState` - Attack active
- `ShrineyGuardSetLoopingAttackState` - Continuous attack
- `BossRandomAttackChoice` - Random attack selection

### MonkeyMage

**Mechanic**: Platform-based boss - ride rotating platforms, butt-bounce on head when force field turns blue

**Child Entities**:
- 6 rotating platforms (`InitCircularPlatformEntity`)
- Force field entity (red = invulnerable, blue = vulnerable)
- Additional boss part (0x244655d)

## Vtables

| Address | Used By |
|---------|---------|
| 0x80011268 | Klogg boss |
| 0x80011288 | JoeHeadJoe boss |
| 0x800112a8 | ShrineyGuard boss |
| 0x800112c8 | GlennYntis boss |
| 0x800112e8 | All HP bar entities |
| 0x80011308 | MonkeyMage boss |
| 0x80011328 | MonkeyMage force field |
| 0x80011348 | MonkeyMage additional part |
| 0x80011428 | JoeHeadJoe special ball |
| 0x80011448 | JoeHeadJoe spiky ball |
| 0x80011468 | JoeHeadJoe regular ball |

## Key Functions Summary

| Function | Address | Purpose |
|----------|---------|---------|
| `BossHPBarTickCallback` | 0x8004992c | Update HP bar display |
| `BossEventHandler` | 0x8004b5fc | Handle damage/animation events (ShrineyGuard) |
| `JoeHeadJoeAttackEventHandler` | 0x8004ca44 | Handle ball events |
| `JoeHeadJoeSelectAttackPattern` | 0x8004ce74 | Choose ball type based on HP |
| `EntitySetState` | 0x8001eaac | Set entity state (tick + event handler) |

## Verification Notes

All patterns verified via Ghidra decompilation of:
- `InitJoeHeadJoeBoss` (0x8004c0e0)
- `InitKloggBoss` (0x8004d88c)
- `InitGlennYntisBoss` (0x80049a54)
- `InitShrineyGuardBoss` (0x8004af64)
- `InitMonkeyMageBoss` (0x80047fb8)
- `BossHPBarTickCallback` (0x8004992c)
- `BossEventHandler` (0x8004b5fc)
- `JoeHeadJoeAttackEventHandler` (0x8004ca44)
