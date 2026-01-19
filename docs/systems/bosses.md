# Boss System Reference

**Last Updated**: January 20, 2026  
**Coverage**: ~97% (80+ functions identified and named)

## Overview

Skullmonkeys features 5 bosses across different levels:

| Boss | Entity Type | Level | HP | Address |
|------|-------------|-------|----|---------| 
| Monkey Mage | 71 | WIZZ (21) | 5 | 0x80047fb8 |
| Glenn Yntis | 100 | YETI (16) | 5 | 0x80049a54 |
| Shriney Guard | 101 | SHRI (23) | 3 | 0x8004af64 |
| Joe-Head-Joe | 102 | JOEJ (11) | 5 | 0x8004c0e0 |
| Klogg | 103 | KLOK (25) | 5 | 0x8004d88c |

---

## Common Boss Architecture

All bosses share common patterns:

### HP System
- HP stored in `g_pPlayerState[0x1D]` (byte at offset 0x1D)
- Bosses initialize HP during `Init*Boss()` function
- Damage handled by `BossEventHandler` (0x8004b5fc)
- Death triggered when HP reaches 0

### Events
- `0x1001`: Upper hit (butt-bounce damage)
- `0x1002`: Lower hit (projectile damage)  
- `0x1008`: Ball register (Joe-Head-Joe specific)

### VTable Pointers
Each boss and sub-entity has a vtable pointer at entity+0x18:

| Boss/Part | VTable |
|-----------|--------|
| Klogg | 0x80011268 |
| Joe-Head-Joe | 0x80011288 |
| Shriney Guard | 0x800112a8 |
| Glenn Yntis | 0x800112c8 |
| Monkey Mage Platforms | 0x800112e8 |
| Monkey Mage Main | 0x80011308 |
| Monkey Mage Force Field | 0x80011328 |
| Monkey Mage Aux Part | 0x80011348 |

---

## Shared Boss Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `BossEventHandler` | 0x8004b5fc | Common damage event handling |
| `BossRandomAttackChoice` | 0x8004ba74 | Random attack pattern selection |
| `BossHPBarTickCallback` | 0x8004992c | HP bar HUD update |
| `BossCollision_SpawnDebrisAndLayers` | 0x800734fc | Death explosion effects |
| `CreateBossPlayerEntity` | 0x80078200 | Boss-specific player setup |
| `EntityEventHandler_SetFlag0x110` | 0x80043ef0 | Set entity flag on event 0x1009 |
| `EntityMoveHorizontalByFacing` | 0x8004f8ac | Simple ±10 pixel horizontal move |
| `GenericEntityDestroyCallback` | 0x8004faf4 | Generic entity destroy (vtable 0x800115a8) |

---

## Boss: Monkey Mage (WIZZ Level 21)

**Entity Type**: 71  
**HP**: 5  
**Init Function**: `InitMonkeyMageBoss` @ 0x80047fb8  
**VTable**: 0x80011308

### Description
Complex platform-based boss. Creates 6 rotating platforms that the player must ride to reach the boss head. Force field mechanic - attacks when orange/red (invulnerable), vulnerable when blue.

### Sub-Entities
- 6 circular platforms (vtable 0x800112e8)
- Force field entity (vtable 0x80011328)
- Additional boss part (vtable 0x80011348)

### Functions (10 named)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitMonkeyMageBoss` | 0x80047fb8 | Initialize boss and sub-entities |
| `EntityType071_MonkeyMageBoss_Init` | 0x80080fec | Entity type dispatch entry |
| `MonkeyMageDeathCallback` | 0x8004ec7c | Death state handler |
| `MonkeyMageDestroyCallback` | 0x8004ec9c | Cleanup main entity |
| `MonkeyMagePartDestroyCallback` | 0x8004edcc | Cleanup boss part |
| `MonkeyMagePlatformDestroyCallback` | 0x8004ee30 | Cleanup platforms |
| `MonkeyMageForceFieldDestroyCallback` | 0x8004ee94 | Cleanup force field |
| `MonkeyMageAuxDestroyCallback` | 0x8004eef8 | Cleanup auxiliary part |
| `MonkeyMageBossPartDestroyCallback` | 0x8004ef5c | Cleanup boss part (alt) |
| `MonkeyMageHUDDestroyCallback` | 0x8004efc0 | Cleanup HUD elements |
| `MonkeyMageSimpleDestroyCallback` | 0x8004f034 | Simple cleanup |

---

## Boss: Glenn Yntis (YETI Level 16)

**Entity Type**: 100  
**HP**: 5  
**Init Function**: `InitGlennYntisBoss` @ 0x80049a54  
**VTable**: 0x800112c8

### Description
Ground-based boss with multiple attack patterns. Phase difficulty increases as HP decreases (calculated as `5 - HP`).

### Functions (8 named)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitGlennYntisBoss` | 0x80049a54 | Initialize boss entity |
| `EntityType100_GlennYntis_Init` | 0x8008105c | Entity type dispatch entry |
| `GlennYntisEventHandler` | 0x80049d54 | Main event processor |
| `GlennYntisDamageEventHandler` | 0x80049ed4 | Damage processing (0x1002) |
| `GlennYntisAttackEventHandler` | 0x8004a0f8 | Attack state events |
| `GlennYntisDeathEventHandler` | 0x8004a3d8 | Death sequence |
| `GlennYntisSetPhaseFromHP` | 0x8004aaf8 | Calculate attack phase from HP |
| `GlennYntisVictoryCallback` | 0x8004aed0 | Victory state |

---

## Boss: Shriney Guard (SHRI Level 23)

**Entity Type**: 101  
**HP**: 3  
**Init Function**: `InitShrineyGuardBoss` @ 0x8004af64  
**VTable**: 0x800112a8

### Description
Guardian boss with attack counters. Has idle, stun, attack, and death states. Uses random attack selection between counter values.

### Functions (16 named)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitShrineyGuardBoss` | 0x8004af64 | Initialize boss entity |
| `EntityType101_ShrineyGuard_Init` | 0x800810cc | Entity type dispatch entry |
| `ShrineyGuardIdleTickCallback` | 0x8004b558 | Idle state tick |
| `ShrineyGuardStunTickCallback` | 0x8004b5b4 | Stun state tick |
| `ShrineyGuardActiveEventHandler` | 0x8004b874 | Active state events |
| `ShrineyGuardAttackEventHandler` | 0x8004b904 | Attack state events |
| `ShrineyGuardMoveCallback` | 0x8004b9a0 | Accelerating horizontal movement |
| `ShrineyGuardAttackCounterState` | 0x8004bac8 | Attack counter management |
| `ShrineyGuardSetAttackState` | 0x8004bb38 | Enter attack state |
| `ShrineyGuardSetLoopingAttackState` | 0x8004bbac | Enter looping attack |
| `ShrineyGuardIdleState` | 0x8004bc90 | Idle state setup |
| `ShrineyGuardAttackAnimState` | 0x8004bd44 | Attack animation state |
| `ShrineyGuardReadyAttackState` | 0x8004bdfc | Ready for attack state |
| `ShrineyGuardStartLoopAttackState` | 0x8004bee0 | Start looping attack |
| `ShrineyGuardDeathState` | 0x8004bfc4 | Death state |
| `ShrineyGuardDeathCallback` | 0x8004c0c0 | Death cleanup |

---

## Boss: Joe-Head-Joe (JOEJ Level 11)

**Entity Type**: 102  
**HP**: 5  
**Init Function**: `InitJoeHeadJoeBoss` @ 0x8004c0e0  
**VTable**: 0x80011288

### Description
Brick-breaker style boss. Spawns balls with varying patterns based on HP phase. Uses attack pattern table at 0x8009bb28 (90 bytes: 5 phases × 18 patterns).

### Ball Types
- 0 = Regular ball
- 1 = Spiky ball (more common at lower HP)
- 2 = Special ball

### Attack Pattern Table (0x8009bb28)
Pattern selection varies by phase (5-HP):
- Phase 0 (5HP): More regular balls
- Phase 4 (1HP): Most spiky balls

### Functions (23 named)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitJoeHeadJoeBoss` | 0x8004c0e0 | Initialize boss entity |
| `EntityType102_JoeHeadJoe_Init` | 0x8008113c | Entity type dispatch entry |
| `JoeHeadJoeUpdateWithCollisionCheck` | 0x8004c4dc | Movement with collision |
| `JoeHeadJoe_CheckAttackAndUpdate` | 0x8004c9e4 | Attack timing check |
| `JoeHeadJoeAttackEventHandler` | 0x8004ca44 | Attack state events |
| `JoeHeadJoeBounceEventHandler` | 0x8004cd90 | Ball bounce events |
| `JoeHeadJoeDeathEventHandler` | 0x8004cdec | Death sequence events |
| `JoeHeadJoeSelectAttackPattern` | 0x8004ce74 | Pattern selection from table |
| `JoeHeadJoeCheckPlayerInAttackRange` | 0x8004cf50 | Range check for attacks |
| `JoeHeadJoeSetIdleState` | 0x8004d138 | Enter idle state |
| `JoeHeadJoeClearVoice` | 0x8004d264 | Stop voice channel |
| `JoeHeadJoeSetFacingAndAttack` | 0x8004d270 | Face player and attack |
| `JoeHeadJoeEnterActiveState` | 0x8004d32c | Enter active combat |
| `JoeHeadJoeMoveAndCheckAttack` | 0x8004d3e8 | Movement with attack check |
| `JoeHeadJoeClearVoiceAlt` | 0x8004d61c | Stop voice (alternate) |
| `JoeHeadJoeReturnToIdleState` | 0x8004d628 | Return to idle |
| `JoeHeadJoeReturnToIdleStateAlt` | 0x8004d6dc | Return to idle (alternate) |
| `JoeHeadJoeDeathAnimState` | 0x8004d790 | Death animation |
| `InitJoeHeadJoeBallRegular` | 0x80053afc | Create regular ball |
| `JoeHeadJoeBallRegularInitState` | 0x80053d70 | Regular ball state init |
| `JoeHeadJoeBallStopSound` | 0x80053edc | Ball sound cleanup |
| `InitJoeHeadJoeBallSpiky` | 0x80054060 | Create spiky ball |
| `InitJoeHeadJoeBallSpecial` | 0x800543c0 | Create special ball |

---

## Boss: Klogg (KLOK Level 25)

**Entity Type**: 103  
**HP**: 5  
**Init Function**: `InitKloggBoss` @ 0x8004d88c  
**VTable**: 0x80011268

### Description
Final boss with butt-bounce damage mechanic. Uses position-based attack patterns. Movement targets read from pattern table at 0x8009ba34.

### Functions (13 named)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitKloggBoss` | 0x8004d88c | Initialize boss entity |
| `EntityType103_Klogg_Init` | 0x800811ac | Entity type dispatch entry |
| `InitKloggBossEntity` | 0x80047288 | Boss entity setup |
| `KloggEventHandlerWithTrigger` | 0x80048ab0 | Event handler with trigger |
| `InitKloggChaseState` | 0x80048e2c | Enter chase player state |
| `KloggDeathCallback` | 0x8004d860 | Death state handler |
| `KloggDestroyCallback` | 0x8004da64 | Entity cleanup |
| `KloggUpdateWithSound` | 0x8004dd30 | Update with sound panning |
| `KloggUpdateWithTimer` | 0x8004dd7c | Update with timer countdown |
| `KloggDeathEventHandler` | 0x8004e148 | Death sequence events |
| `KloggMoveToTargetPosition` | 0x8004e570 | Move to pattern target |
| `KloggSpawnProjectilesCallback` | 0x8004e680 | Spawn attack projectiles |
| `KloggSetMoveState` | 0x8004e920 | Enter movement state |

---

## Data Tables

### Boss Sprite Tables
- `g_MonkeyMageBossSprites` - Monkey Mage sprite IDs
- `g_KloggBossSprites` - Klogg sprite IDs  
- `g_ShrineyGuardBossSprites` - Shriney Guard sprite IDs
- `g_GlennYntisBossSprites` - Glenn Yntis sprite IDs
- `g_JoeHeadJoeBossSprites` - Joe-Head-Joe sprite IDs

### Attack Pattern Tables
| Address | Boss | Size | Description |
|---------|------|------|-------------|
| 0x8009bb28 | Joe-Head-Joe | 90 bytes | 5 phases × 18 ball patterns |
| 0x8009ba34 | Klogg | variable | Position targets array |
| 0x8009b860 | Monkey Mage | 24 bytes | Platform relative positions (6 × 4) |
| 0x8009bb04 | Glenn Yntis | variable | Attack spawn offsets |
| 0x8009bba8 | Klogg | variable | Debris spawn offsets |

---

## Summary Statistics

| Boss | Functions Named | Estimated Total | Coverage |
|------|-----------------|-----------------|----------|
| Monkey Mage | 11 | ~12 | ~92% |
| Glenn Yntis | 8 | ~8 | ~100% |
| Shriney Guard | 16 | ~16 | ~100% |
| Joe-Head-Joe | 23 | ~25 | ~92% |
| Klogg | 13 | ~15 | ~87% |
| Shared | 8 | ~8 | ~100% |
| **Total** | **80** | **~84** | **~95%** |

---

## Related Documentation

- [Entity System](entities.md) - Entity spawn and lifecycle
- [Collision System](collision-complete.md) - Tile collision for boss arenas
- [Player Physics](player/player-physics.md) - Butt-bounce mechanics
- [Animation Framework](animation-framework.md) - Boss animation states
