# Official Enemy Names from Game Manual

**Source**: Skullmonkeys Official Game Manual  
**Last Updated**: January 2026  
**Purpose**: Map entity types to official enemy names from the manual

---

## Bosses (5 Total)

All bosses have 5 HP (except Shriney Guard with 3 HP).

| Level | Code | Official Name | Entity Type | Ghidra Function | HP |
|-------|------|---------------|-------------|-----------------|-----|
| 5 | MEGA | **Shriney Guard** | 71 (spawns boss) | EntityType071_BossSpawner_Init | 3 |
| 9 | HEAD | **Joe-Head-Joe** | 71 (spawns boss) | EntityType071_BossSpawner_Init | 5 |
| 15 | GLEN | **Glenn Yntis** | 71 (spawns boss) | EntityType071_BossSpawner_Init | 5 |
| 21 | WIZZ | **Monkey Mage** | 71 (spawns boss) | EntityType071_BossSpawner_Init | 5 |
| 24 | KLOG | **Klogg** | 71 (spawns boss) | EntityType071_BossSpawner_Init | 5 |

**Key Functions:**
- `InitBossEntity` @ 0x80047fb8 - Creates boss with 6 circular parts
- `EntityType071_BossSpawner_Init` @ 0x80080fec - Spawns boss entity

---

## Standard Enemies from Game Manual

### Ground Enemies (Butt-Bounceable)

| Official Name | Description | Weakness | Behavior | Possible Entity Type |
|---------------|-------------|----------|----------|---------------------|
| **Clay Keeper** | Skullmonkeys that snack on clay balls | Butt-bounce (free clayball drop) | Sits still or lumbers back and forth | 25 (Ground Patrol) |
| **Loud Mouth** | Monkeys that run around flailing arms | Butt-bounce | Long reach, geeky run | 25 (variant) |
| **Mental Monkey** | Similar to Loud Mouth but faster | Carefully timed butt-bounce | Double speed when flailing | 17-22 (Enemy Cluster) |
| **Tempest Pulsating Monkey** | Enemy that periodically glows | Wait for calm, then bounce or shoot | Electricity when glowing | 17-22 (Enemy Cluster) |
| **Jumpy the Gorilla** | Large monkey that jumps | Run under or butt-bounce | Constant jumping | 17-22 (Enemy Cluster) |
| **Swarm-o-Ynts** | Baby Ynts in groups | Butt-bounce | Travel in packs | 27 (Path Enemy) |
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

## Entity Type to Enemy Category Mapping

Based on Ghidra analysis:

| Entity Type | Category | Official Enemy Names | Ghidra Function |
|-------------|----------|---------------------|-----------------|
| 2 | Collectible | Clayball | EntityType002_Clayball_Init |
| 3 | Pickup | Ammo | EntityType004_DefaultPickup_Init |
| 5 | Flying Enemy | Flapper, Flying Ynt, etc. | EntityType005_FlyingEnemy_Init |
| 6 | Flying Enemy Alt | Flying variants | EntityType006_FlyingEnemyAlt_Init |
| 8 | Item Pickup | 1-Up, power-ups | EntityType008_ItemPickup_Init |
| 17-22 | Enemy Cluster | Mental Monkey, Tempest, Jumpy | EntityType017-022_EnemyCluster_Init |
| 25 | Ground Patrol | Clay Keeper, Loud Mouth | EntityType025_GroundPatrolEnemy_Init |
| 27 | Path Enemy | Swarm-o-Ynts, Worker Ynt | EntityType027_PathEnemy_Init |
| 28 | Platform | Moving platforms | EntityType028_PlatformA_Init |
| 42 | Portal | Level transitions | EntityType042_Portal_Init |
| 45 | Message Box | Save points | EntityType045_MessageBox_Init |
| 48 | Platform B | Vertical platforms | EntityType048_PlatformB_Init |
| 49-51 | Boss Parts | Boss components | EntityType049-051_Boss*_Init |
| 61 | Effect | Sparkle decoration | EntityType061_Sparkle_Init |
| 71 | Boss Spawner | All 5 bosses | EntityType071_BossSpawner_Init |
| 95 | 1970s Bonus | SEVN level collectibles | EntityType095_1970sBonus_Init |

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

1. **Entity Type 25** (GroundPatrolEnemy) likely covers multiple manual enemies: Clay Keeper, Loud Mouth, and similar patrol-type enemies using variant data.

2. **Entity Types 17-22** (EnemyCluster) share similar callbacks and likely represent different behavioral variants of ground enemies.

3. **Boss entities** all use the same spawner (Type 71) which calls InitBossEntity. The specific boss is determined by level data.

4. **Invulnerable enemies** like Robot Hover Monkey and Screaming Inferno have special handling that prevents damage.

5. **Weapon-only enemies** like Egg-Beater and Castle Trooper have collision flags that prevent butt-bounce damage.
