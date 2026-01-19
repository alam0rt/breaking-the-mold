# Official Enemy Names from Game Manual

**Source**: Skullmonkeys Official Game Manual  
**Last Updated**: January 2026  
**Purpose**: Map entity types to official enemy names from the manual

---

## Bosses (5 Total)

All bosses have 5 HP (except Shriney Guard with 3 HP).

| Level | Code | Official Name | BLB Type | Internal Type | Init Function | HP |
|-------|------|---------------|----------|---------------|---------------|-----|
| 5 | MEGA | **Shriney Guard** | 142 (L1) | 101 | EntityType101_ShrineyGuard_Init | 3 |
| 9 | HEAD | **Joe-Head-Joe** | 144 (L1) | 102 | EntityType102_JoeHeadJoe_Init | 5 |
| 15 | GLEN | **Glenn Yntis** | 143 (L1) | 100 | EntityType100_GlennYntis_Init | 5 |
| 21 | WIZZ | **Monkey Mage** | 141 (L1) | 71 | EntityType071_MonkeyMageBoss_Init | 5 |
| 24 | KLOG | **Klogg** | 145 (L1) | 103 | EntityType103_Klogg_Init | 5 |

**Boss Remapping (Layer 1):**
- BLB 141 (0x8d) → Internal 71 (0x47) = Monkey Mage
- BLB 142 (0x8e) → Internal 101 (0x65) = Shriney Guard
- BLB 143 (0x8f) → Internal 100 (0x64) = Glenn Yntis
- BLB 144 (0x90) → Internal 102 (0x66) = Joe-Head-Joe
- BLB 145 (0x91) → Internal 103 (0x67) = Klogg

**Key Functions:**
- `InitMonkeyMageBoss` @ 0x80047fb8 - Creates boss with 6 circular parts
- `InitGlennYntisBoss` @ 0x80049a54 - Glenn Yntis boss logic
- `InitShrineyGuardBoss` @ 0x8004af64 - Shriney Guard boss logic (3 HP)
- `InitJoeHeadJoeBoss` @ 0x8004c0e0 - Joe-Head-Joe boss logic
- `InitKloggBoss` @ 0x8004d88c - Final boss Klogg logic
- `BossHPBarTickCallback` @ 0x8004992c - HP bar display
- `BossEventHandler` @ 0x8004b5fc - Boss damage/death handling

---

## Standard Enemies from Game Manual

### Ground Enemies (Butt-Bounceable)

| Official Name | Description | Weakness | Behavior | BLB Type | Status |
|---------------|-------------|----------|----------|----------|--------|
| **Loud Mouth** | Monkeys that run around flailing arms | Butt-bounce | Long reach, geeky run | **BLB 25** | ✅ CONFIRMED |
| **Clay Keeper** | Skullmonkeys that snack on clay balls | Butt-bounce (free clayball drop) | Sits still or lumbers back and forth | Unknown | ❓ |
| **Mental Monkey** | Similar to Loud Mouth but faster | Carefully timed butt-bounce | Double speed when flailing | Unknown | ❓ |
| **Tempest Pulsating Monkey** | Enemy that periodically glows | Wait for calm, then bounce or shoot | Electricity when glowing | Unknown | ❓ |
| **Jumpy the Gorilla** | Large monkey that jumps | Run under or butt-bounce | Constant jumping | Unknown | ❓ |
| **Swarm-o-Ynts** | Baby Ynts in groups | Butt-bounce | Travel in packs | Unknown | ❓ |

### Verified Entity Mappings

| BLB Type | Official Name | Levels Found | Verification |
|----------|---------------|--------------|--------------|
| **25** | **Loud Mouth** | SCIE stage 0 (8x) | Visual confirmation Jan 2026 |
| **27** | Unknown | SCIE stage 0 (8x) | Often near BLB 25, purpose unclear |
| **8** | Halo (1-up item) | SCIE stage 0 | Visual confirmation - NOT an enemy |
| **Worker Ynt** | Insect-like scuttlers | Moving quickly or standard attacks | Scuttle and scratch | 27 (Path Enemy) |
| **El Barfo** | Skulls that barf themselves | Butt-bounce or weapons | Deadly without skin | Unknown |

### Flying Enemies

| Official Name | Description | Weakness | Behavior | Possible Entity Type |
|---------------|-------------|----------|----------|---------------------|
| **Flapper** | Monkeys with hand fans | Quick butt-bounce | Flies above, regenerates quickly | 5, 6 (Flying Enemy) |
| **Flying Ynt Centurion** | Spiked insect guards | Weapons only (spikes on back) | Giant spikes prevent bounce | 5, 6 (Flying Enemy) |
| **Evil Engine Royal Guard** | Klogg's golden monkeys | Standard attacks | Flying, protect Evil Engine | 5, 6 (Flying Enemy) |
| **Super Bomber Monk** | Skullmonkey air force | Virtually impossible | Drops bombs on Drivey Runn | Unknown |

### Projectile Enemies

| Official Name | Description | Weakness | Behavior | Possible Entity Type |
|---------------|-------------|----------|----------|---------------------|
| **Robot Hover Monkey** | Indestructible hover dudes | Powerful weapons or avoid | Drops deadly laser balls | Invulnerable entity |
| **Head Shooter Monkey** | Heads fly off | Butt-bounce, blast, or avoid | Head floats and chews | Unknown |
| **Fork Shooter Monkey** | Hot Dog Factory guards | Defeat before they stab | Giant fork launchers | Unknown |
| **Sno-Blo** | Arctic Skullmonkeys | Fast reactions | Shoots icy snowballs | Unknown |

### Special Enemies (Weapon Required)

| Official Name | Description | Weakness | Behavior | Possible Entity Type |
|---------------|-------------|----------|----------|---------------------|
| **Egg-Beater** | Propeller-head monkey | Weapons only (razor propeller) | Cannot be butt-bounced | Unknown |
| **Castle Trooper** | Stained glass guards | Jump past or weapons (not bounce!) | Holds spear upward | Unknown |
| **Pipe-Cleaners** | Big-mouthed worms | Jump or weapons (tough hide) | Tongue flicks from pipes | Unknown |

### Invulnerable Hazards

| Official Name | Description | Weakness | Behavior |
|---------------|-------------|----------|----------|
| **Screaming Inferno** | Floating flaming skull | None (invulnerable) | Floats between Idznak and underworld |
| **Pop-Corn Skulls** | Tiny sewer monkeys | Unknown | Pops from holes to bite |
| **Triple Laser Butt Bounce Monkey** | Death trap enemy | Bounce/weapons (avoid laser balls) | Leaves 3 laser balls on death |

### Special Entities

| Official Name | Description | Entity Type | Ghidra Function |
|---------------|-------------|-------------|-----------------|
| **Ma Bird** | Checkpoint (butt-bounce to save) | Unknown | Checkpoint system |
| **Barking Bird** | Blind birds | Unknown | Harmless when big, deadly when shrunk |

---

## Entity Type Remapping System

Entity types in BLB files are **layer-dependent** and get remapped to internal types by `RemapEntityTypesForLevel` @ 0x8008150c.

### Layer Definitions
- **Layer 1**: Foreground entities (player-interactive, enemies)
- **Layer 2**: Background entities (collectibles, decorations, some enemies)
- **Layer 3**: Special entities (triggers, mechanisms, overlays)

### Internal Entity Type → Init Function Mapping

Based on callback table at 0x8009D5F8 (121 entries):

| Internal | Category | Init Function | Description |
|----------|----------|---------------|-------------|
| 0 | Pickup | EntityType004_DefaultPickup_Init | Generic pickup (via L1 0x19) |
| 1 | Boss | EntityType001_BossEntity_Init | Boss state entity |
| 2 | Clayball | EntityType002_Clayball_Init | Standard clayball collectible |
| 3-4 | Pickup | EntityType004_DefaultPickup_Init | Ammo/powerup pickups |
| 5 | Enemy | EntityType005_FlyingEnemy_Init | Flying enemy (L3 0x1c) |
| 6 | Enemy | EntityType006_FlyingEnemyAlt_Init | Flying enemy variant (L3 0x08) |
| 7 | Item | EntityType007_ItemCollectible_Init | Item collectible |
| 8 | Pickup | EntityType008_ItemPickup_Init | Default/unknown entity |
| 9 | Collect | EntityType009_Collectible_Init | Collectible variant |
| 10 | Object | EntityType010_InteractiveObject_Init | Interactive object |
| 11-12 | Collect | Collectible_Init variants | Various collectibles |
| 17-23 | Enemy | EnemyCluster_Init (17-23) | Ground enemy types |
| 24 | Pickup | EntityType118_SpecialPickup_Init | Special pickup (L2 0x18, L1 0x2d) |
| 25 | Enemy | EntityType025_GroundPatrolEnemy_Init | Ground patrol enemy |
| 26 | Enemy | EntityType026_Enemy_Init | Path-following enemy |
| 27 | Enemy | EntityType027_PathEnemy_Init | Path enemy variant |
| 28 | Platform | EntityType028_PlatformA_Init | Horizontal platform |
| 29 | Enemy | EntityType029_Enemy_Init | Enemy variant |
| 30 | Decor | EntityType083_InteractiveDecor_Init | Interactive decoration |
| 31-36 | Object | ObjectVariant_Init | Object variants |
| 37-41 | Mechanism | Mechanism_Init variants | Level mechanisms |
| 42-44,53-55,60 | Portal | EntityType042_Portal_Init | Level portals/exits |
| 45 | Message | EntityType045_MessageBox_Init | Save point/message |
| 46 | Object | EntityType046_Object_Init | Generic object |
| 47-48 | Platform | EntityType048_PlatformB_Init | Vertical platforms |
| 49-51 | Boss | Boss*_Init | Boss component entities |
| 52 | Mechanism | EntityType052_Mechanism_Init | Mechanism variant |
| 57-59 | Waypoint | WaypointClayball*_Init | Waypoint clayballs |
| 61 | Effect | EntityType061_Sparkle_Init | Visual sparkle effect |
| 62-68 | Clayball | ClayballVariant*_Init | Clayball variants |
| 69 | Decor | EntityType069_FadeableDecor_Init | Fadeable decoration |
| 70 | Decor | EntityType070_PathDecor_Init | Path-following decoration |
| 71 | **Boss** | EntityType071_MonkeyMageBoss_Init | **Monkey Mage boss** |
| 72 | Decor | EntityType072_AnimatedDecor_Init | Animated decoration |
| 75 | Decor | EntityType075_SimpleDecor_Init | Simple decoration |
| 76 | Enemy | EntityType076_PathEnemy_Init | Path enemy variant |
| 79-83 | Menu | Menu/Timer_Init variants | Menu entities |
| 84 | Enemy | EntityType084_PathEnemyAlt_Init | Path enemy alt |
| 85 | Trigger | EntityType105_TriggerZone_Init | Trigger zone |
| 86-88 | Decor | EntityType088_ForegroundDecor_Init | Foreground decoration |
| 89 | Grid | EntityType111_GridLine_Init | Grid line |
| 90-93 | Clayball | SwitchClayball*_Init | Switch-triggered clayballs |
| 94 | Sprite | EntityType094_IndexedSprite_Init | Indexed sprite |
| 95 | Bonus | EntityType095_1970sBonus_Init | 1970s icon (SEVN unlock) |
| 96 | Enemy | EntityType096_PathPlatformEnemy_Init | Platform path enemy |
| 97-98,110-111 | Grid | EntityType111_GridLine_Init | Grid lines |
| 99 | Collect | EntityType099_LevelStateCollectible_Init | Level state collectible |
| 100 | **Boss** | EntityType100_GlennYntis_Init | **Glenn Yntis boss** |
| 101 | **Boss** | EntityType101_ShrineyGuard_Init | **Shriney Guard boss** |
| 102 | **Boss** | EntityType102_JoeHeadJoe_Init | **Joe-Head-Joe boss** |
| 103 | **Boss** | EntityType103_Klogg_Init | **Klogg final boss** |
| 104-105 | Trigger | EntityType105_TriggerZone_Init | Trigger zones |
| 106-108 | Platform | EntityType108_VerticalPlatform_Init | Vertical platforms |
| 109 | Collect | EntityType109_TimedCollectible_Init | Timed collectible |
| 112-114 | Decor | EntityType114_DecorationA_Init | Decoration A variants |
| 115-117 | Decor | EntityType117_DecorationB_Init | Decoration B variants |
| 118 | Pickup | EntityType118_SpecialPickup_Init | Special pickup |
| 119 | Decor | EntityType081_PathDecorAlt_Init | Path decoration alt |
| 120 | Sprite | EntityType120_ChildSprite_Init | Child sprite entity |

### Enemy Entity Types

| Internal Type | Init Function | Official Enemy Names | Behavior |
|---------------|---------------|---------------------|----------|
| 5 | FlyingEnemy_Init | Flapper | Flying, fan-based |
| 6 | FlyingEnemyAlt_Init | Flying Ynt Centurion | Flying, spiked |
| 17-23 | EnemyCluster_Init | Mental Monkey, Tempest Pulsating Monkey, Jumpy the Gorilla | Various ground behaviors |
| 25 | GroundPatrolEnemy_Init | Clay Keeper, Loud Mouth | Ground patrol, butt-bounceable |
| 26-27 | PathEnemy_Init | Worker Ynt, Swarm-o-Ynts | Path following |
| 29 | Enemy_Init | Various | General enemy |
| 76 | PathEnemy_Init | Path variant | Path following |
| 84 | PathEnemyAlt_Init | Path variant | Path following |
| 96 | PathPlatformEnemy_Init | Platform-based | Rides platforms |

---

## Weapons Reference

| Weapon | Button | Max | Description |
|--------|--------|-----|-------------|
| **Green Bullets** | Circle | 20 | Standard projectile ammo |
| **Phoenix Hand** | L1 | 7 | Homing bird attack |
| **Phart Head** | L2 | 7 | Ghostly green clone (scout) |
| **Universe Enema** | R1 | 7 | Screen-wide destruction |

---

## Defensive Items

| Item | Max | Description |
|------|-----|-------------|
| **Halo** | 1 | Single-hit shield |
| **Hamsters** | 3 | Orbiting shields (3 extra hits, wander off) |
| **Glidey Bird** | 1 | Controlled descent (hold X) |
| **Super Willie** | 7 | R2 to auto-collect all items |

---

## Notes

1. **Entity Type Remapping**: BLB entity types are layer-dependent. The same BLB type on different layers maps to different internal types. Function `RemapEntityTypesForLevel` @ 0x8008150c handles this.

2. **Boss Entities**: Each boss has a unique internal type (71, 100-103) with dedicated init function. NOT a shared spawner.

3. **Entity Type 25** (GroundPatrolEnemy) covers multiple manual enemies: Clay Keeper, Loud Mouth, and similar patrol-type enemies using variant data.

4. **Entity Types 17-23** (EnemyCluster) represent different behavioral variants of ground enemies including Mental Monkey, Tempest, and Jumpy.

5. **Flying Enemies** (Types 5-6) map from Layer 3 BLB types 0x1c and 0x08 respectively.

6. **Default Handler**: Unknown entity types default to Internal Type 8 (EntityType008_ItemPickup_Init).

7. **1970s Bonus** (Type 95): BLB types 201-228 on Layer 2 map to this. Used for SEVN level unlock collectibles.

8. **Callback Table**: 121 entries at 0x8009D5F8, 8 bytes each (2 bytes flags + padding + 4 byte function pointer).

