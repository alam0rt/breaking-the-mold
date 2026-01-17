# Boss System Analysis

## Overview

Skullmonkeys has **5 bosses** across different levels. Through code analysis, we've identified that bosses fall into **4 distinct systems**:

| Boss | Level | World | BLB Type | Internal Type | HP | System Type |
|------|-------|-------|----------|---------------|----|----|
| **Shriney Guard** | MEGA (5) | 1 | 142 | 101 | **3** | Ball-Throwing (variant A) |
| **Joe-Head-Joe** | HEAD (9) | 2 | 144 | 102 | 5 | Ball-Throwing (variant B) |
| **Glenn Yntis** | GLEN (15) | 3 | 143 | 100 | 5 | Hazard Boss |
| **Monkey Mage** | WIZZ (21) | 4 | 141 | 71 | 5 | Multi-Entity Boss |
| **Klogg** | KLOG (24) | 5 | 145 | 103 | 5 | Enemy-Style Boss |

## Boss System Types

### 1. Ball-Throwing System (Shriney Guard)

**Init Function:** `InitShrineyGuardBoss` @ 0x8004af64
**Entity Type:** 101

Uses `BossEventHandler` (0x8004b5fc) which handles:
- Event 0x1001/0x1002: Ball hit boss → decrement HP
- Event 0x1008: Ball registration
- Event 0x0001 + markers: Animation effects

**State Machine:**
```
BossRandomAttackChoice → ShrineyGuardSetAttackState
                       → ShrineyGuardSetLoopingAttackState

ShrineyGuardAttackAnimState → ShrineyGuardReadyAttackState
                            → ShrineyGuardIdleState
                            
On death → ShrineyGuardDeathState
```

**Key Features:**
- Only boss with 3 HP (all others have 5)
- Simple rolling attack pattern
- Uses `BossEventHandler` callback at entity+0xc

---

### 2. Ball-Throwing System (Joe-Head-Joe) 

**Init Function:** `InitJoeHeadJoeBoss` @ 0x8004c0e0
**Entity Type:** 102

Uses `JoeHeadJoeAttackEventHandler` (0x8004ca44) which spawns projectiles:
- entity+0x114 == 0: `InitJoeHeadJoeBallRegular` (catchable ball)
- entity+0x114 == 1: `InitJoeHeadJoeBallSpiky` (hazard, count varies by HP)
- entity+0x114 == 2: `InitJoeHeadJoeBallSpecial` (special ball)

**State Machine:**
```
JoeHeadJoeSetIdleState → JoeHeadJoeAttackEventHandler
                       → JoeHeadJoeSetFacingAndAttack
                       
On hit → JoeHeadJoeEnterActiveState
On death → JoeHeadJoeDeathAnimState

Return → JoeHeadJoeReturnToIdleState
```

**Key Features:**
- Uses `ApplyAnimationPositionOffsets` callback
- More complex ball mechanics than Shriney Guard
- Ball count scales with remaining HP

---

### 3. Hazard Boss System (Glenn Yntis)

**Init Function:** `InitGlennYntisBoss` @ 0x80049a54
**Entity Type:** 100

Uses `HazardActivateWithSound` (0x8004a768) as initial state.

**State Machine:**
```
HazardActivateWithSound → plays sound 0x7a318245
                        → sets sprite 0x8068815c
                        → HazardStopSound callback
```

**Key Features:**
- Different event handler at LAB_80049d54
- Uses `ApplyAnimationPositionOffsets` callback
- Simplest state machine

---

### 4. Multi-Entity Boss System (Monkey Mage)

**Init Function:** `InitMonkeyMageBoss` @ 0x80047fb8
**Entity Type:** 71

Spawns multiple sub-entities:
- 6 rotating platforms (sprite 0x8818a018)
- Force field entity (via `InitEntityWithSprite`)
- Additional boss part (sprite 0x244655d)

**Structure:**
```
entity+0x114 through +0x128: Platform pointers (6 × 4 bytes)
entity+0x130: Force field entity pointer
entity+0x134: Additional boss part pointer
```

**Platform Mechanics:**
- Uses `InitCircularPlatformEntity` for each platform
- Platforms orbit around boss center
- Offset data at 0x8009b860/0x8009b862

**Key Features:**
- Most complex boss
- Force field state determines vulnerability
- Player rides platforms to reach boss

---

### 5. Enemy-Style Boss System (Klogg)

**Init Function:** `InitKloggBoss` @ 0x8004d88c
**Entity Type:** 103

Uses `EnemyIdleTimerState` (0x8004e9f4) → `EnemyHitMessageHandler`

**State Machine:**
```
EnemyIdleTimerState → sets timer 0xB4 (180 frames)
                    → EnemyHitMessageHandler for damage
                    → sprite 0x193ca112
```

**Key Features:**
- Uses standard enemy butt-bounce damage mechanic
- Plays spawn sound 0x486492c4
- Simplest boss from code perspective
- NOT the "brick breaker" boss - that's Joe-Head-Joe!

---

## Function Address Reference

### Shriney Guard (Type 101)
| Address | Function |
|---------|----------|
| 0x8004af64 | InitShrineyGuardBoss |
| 0x8004b5fc | BossEventHandler |
| 0x8004ba74 | BossRandomAttackChoice |
| 0x8004bac8 | ShrineyGuardAttackCounterState |
| 0x8004bb38 | ShrineyGuardSetAttackState |
| 0x8004bbac | ShrineyGuardSetLoopingAttackState |
| 0x8004bc90 | ShrineyGuardIdleState |
| 0x8004bd44 | ShrineyGuardAttackAnimState |
| 0x8004bdfc | ShrineyGuardReadyAttackState |
| 0x8004bee0 | ShrineyGuardStartLoopAttackState |
| 0x8004bfc4 | ShrineyGuardDeathState |
| 0x800810cc | EntityType101_ShrineyGuard_Init |

### Joe-Head-Joe (Type 102)
| Address | Function |
|---------|----------|
| 0x8004c0e0 | InitJoeHeadJoeBoss |
| 0x8004c4dc | JoeHeadJoeUpdateWithCollisionCheck |
| 0x8004ca44 | JoeHeadJoeAttackEventHandler |
| 0x8004ce74 | JoeHeadJoeSelectAttackPattern |
| 0x8004cf50 | JoeHeadJoeCheckPlayerInAttackRange |
| 0x8004d138 | JoeHeadJoeSetIdleState |
| 0x8004d264 | JoeHeadJoeClearVoice |
| 0x8004d270 | JoeHeadJoeSetFacingAndAttack |
| 0x8004d32c | JoeHeadJoeEnterActiveState |
| 0x8004d61c | JoeHeadJoeClearVoiceAlt |
| 0x8004d628 | JoeHeadJoeReturnToIdleState |
| 0x8004d6dc | JoeHeadJoeReturnToIdleStateAlt |
| 0x8004d790 | JoeHeadJoeDeathAnimState |
| 0x80053afc | InitJoeHeadJoeBallRegular |
| 0x80053d70 | JoeHeadJoeBallRegularInitState |
| 0x80053edc | JoeHeadJoeBallStopSound |
| 0x80054060 | InitJoeHeadJoeBallSpiky |
| 0x800543c0 | InitJoeHeadJoeBallSpecial |
| 0x8008113c | EntityType102_JoeHeadJoe_Init |

### Glenn Yntis (Type 100)
| Address | Function |
|---------|----------|
| 0x80049a54 | InitGlennYntisBoss |
| 0x8004a768 | HazardActivateWithSound |
| 0x8008105c | EntityType100_GlennYntis_Init |

### Monkey Mage (Type 71)
| Address | Function |
|---------|----------|
| 0x80047fb8 | InitMonkeyMageBoss |
| 0x80080fec | EntityType071_MonkeyMageBoss_Init |

### Klogg (Type 103)
| Address | Function |
|---------|----------|
| 0x8004d88c | InitKloggBoss |
| 0x8004e9f4 | EnemyIdleTimerState |
| 0x800811ac | EntityType103_Klogg_Init |

---

## Sprite IDs

| Boss | Main Sprite | HUD Sprite |
|------|-------------|------------|
| Shriney Guard | (from BLB) | 0xa8482860 |
| Joe-Head-Joe | (from BLB) | 0x6101db0 |
| Glenn Yntis | (from BLB) | 0x48d81135 |
| Monkey Mage | (from BLB) | 0x181c3854 |
| Klogg | (from BLB) | 0xcaadc032 |

---

## Historical Note: Naming Corrections

Many functions were previously named with "Klogg" prefix based on early analysis. This was **incorrect**:

- The "ball-throwing" boss functions are actually for **Joe-Head-Joe**
- The actual **Klogg** final boss uses a much simpler enemy-style damage system
- **Shriney Guard** has its own variant of the ball-throwing system

The function names have been corrected in Ghidra to reflect actual boss usage.
