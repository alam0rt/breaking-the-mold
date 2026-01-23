# Official Names from Game Manual

**Source**: Skullmonkeys Official Game Manual + GameFAQs Walkthrough  
**Last Updated**: January 24, 2026  
**Purpose**: Map entity types to official names from the manual for visual verification

---

## ⚠️ VERIFICATION STATUS

Items marked with:
- ✅ **Verified**: Sprite visually confirmed by developer
- ⏳ **Pending**: Internal type known, sprite needs visual confirmation  
- ❓ **Unknown**: Not yet mapped to internal type

---

## Bosses (5 Total)

| Level | Code | Official Name | Internal Type | Init Function | HP | Status |
|-------|------|---------------|---------------|---------------|-----|--------|
| MEGA | 5 | **Shriney Guard** | 101 | 0x8004af64 | 3 | ✅ |
| HEAD | 9 | **Joe-Head-Joe** | 102 | 0x8004c0e0 | 5 | ✅ |
| GLEN | 15 | **Glenn Yntis** | 100 | 0x80049a54 | 5 | ✅ |
| WIZZ | 21 | **Monkey Mage** | 71 | 0x80047fb8 | 5 | ✅ |
| KLOG | 24 | **Klogg** | 103 | 0x8004d88c | 5 | ✅ |

**Boss Descriptions from Manual:**
- **Shriney Guard**: Attacks by spitting/"hocking a loogey"
- **Joe-Head-Joe**: Horrific monkey in sewers, uses fire breath and rolling eyeballs
- **Glenn Yntis**: Massive Ynt with giant claws, fight using turret
- **Monkey Mage**: Mystic boss with force field, summons energy beams
- **Klogg**: Final boss operating "Skull Machine", fires spiked balls

---

## Enemies - Standard Monkeys

### Ground Enemies (Butt-Bounceable)

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Clay Keeper** | Sit still or lumber back and forth, snacking on clay | Butt-bounce (drops free clay ball) | ? | ? | ❓ |
| **Loud Mouth** | Run around flailing arms, long reach | Butt-bounce | ? | ? | ❓ |
| **Mental Monkey** | Faster Loud Mouth, double speed when flailing | Carefully timed butt-bounce | ? | ? | ❓ |
| **Tempest Pulsating Monkey** | Glow periodically, "fry" you if bounced while glowing | Wait for calm, then bounce/shoot | ? | ? | ❓ |
| **Jumpy the Gorilla** | Large monkey that jumps constantly | Run under or butt-bounce | ? | ? | ❓ |
| **Triple Laser Butt Bounce Monkey** | Leave 3 glowing laser balls on death | Bounce/weapons (avoid laser balls) | ? | ? | ❓ |
| **El Barfo** | Barf themselves out of their own skin | ? | ? | ? | ❓ |

### Flying Enemies

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Flapper** | Fly using hand fans, regenerate quickly | Quick butt-bounce | 5 or 6? | ? | ❓ |
| **JX1137 Test Pilot** | Monkey with rocketpack | ? | ? | ? | ❓ |
| **Super Bomber Monk** | Pilots dropping bombs on Drivey Runn | Virtually impossible | ? | RUNN | ❓ |
| **Evil Engine Royal Guard** | Elite flying golden monkeys | Standard attacks | ? | EVIL | ❓ |

### Projectile Enemies

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Robot Hover Monkey** | Indestructible, drop deadly laser balls | Powerful weapons or avoid | ? | Early levels | ❓ |
| **Head Shooter Monkey** | Launch own heads that float and bite | Butt-bounce, blast, or avoid | ? | ? | ❓ |
| **Fork Shooter Monkey** | Guards with giant fork launchers | Defeat before they stab | ? | FOOD | ❓ |
| **Sno-Blo** | Arctic monkeys, shoot icy projectiles | Fast reactions | ? | SNOW | ❓ |

### Special Enemies (Weapon Required)

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Egg-Beater** | Razor-sharp propellers on head | Weapons only (no butt-bounce) | ? | ? | ❓ |
| **Castle Trooper** | Guards in stained glass windows with spears | Jump past or weapons (no bounce) | ? | CSTL | ❓ |
| **Pipe-Cleaners** | Big-mouthed worms, flick tongue from pipes | Jump or weapons (tough hide) | ? | ? | ❓ |

### Invulnerable Hazards

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Screaming Inferno** | Flaming floating skull | None (invulnerable) | ? | ? | ❓ |
| **Pop-Corn Skulls** | Tiny monkeys in Hot Dog Factory, pop from holes | Unknown | ? | FOOD | ❓ |

---

## Enemies - Non-Monkey Creatures

### Ynts (Bug Enemies)

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Worker Ynt** | Scuttling bugs that eat anything slow | ? | ? | YNTE levels | ❓ |
| **Flying Ynt Centurion** | Giant spikes on back prevent butt-bounce | Weapons only | 5 or 6? | YNTE levels | ❓ |
| **Swarm-o-Ynts** | Packs of baby Ynts | Butt-bounce | ? | YNTE levels | ❓ |

### Birds and Other

| Official Name | Manual Description | Weakness | Internal Type | Levels | Status |
|---------------|-------------------|----------|---------------|--------|--------|
| **Barking Bird** | Blind birds; harmless when big, deadly when shrunk | Avoid when shrunk | ? | ? | ❓ |

---

## Collectibles

### Core Collectibles

| Official Name | Manual Description | Button | Max | Internal Type | Status |
|---------------|-------------------|--------|-----|---------------|--------|
| **Clay** (Orange Balls) | Floating orbs, 100 = extra life | Auto | - | 2 (Clayball) | ⏳ |
| **Klaymen's Head** (1up) | Instant extra life | Auto | - | ? | ❓ |
| **Swirly Qs** | 3 = bonus room access | Auto | 3 | ? | ❓ |
| **1970s Icons** | 3 total in game, unlock secret room | Auto | 3 | 95 | ⏳ |

### Offensive Weapons

| Official Name | Manual Description | Button | Max | Internal Type | Status |
|---------------|-------------------|--------|-----|---------------|--------|
| **Green Bullets** (Energy Balls) | Standard ammo for projectile attack | Circle | 9-20 | 3 or 4? | ❓ |
| **Phoenix Hand** (The Bird) | Seeks and destroys nearest enemy | L1 | 7 | ? | ❓ |
| **Universe Enema** (Super Power) | Destroys all enemies on screen | R1 | 7 | ? | ❓ |
| **Phart Head** (Fart Clone) | Gas clone, acts as continue point | L2 | 7 | 25 | ⏳ |

### Defensive Items

| Official Name | Manual Description | Button | Max | Internal Type | Status |
|---------------|-------------------|--------|-----|---------------|--------|
| **Halo** | Absorbs one hit, extra = clay | Auto | 1 | 8? | ❓ |
| **Hamster Shield** | 3 hamsters circle, kill 3 enemies | Auto | 3 | ? | ❓ |
| **Glidey Bird** (Yellow Bird) | Allows gliding when holding X | Auto | 1 | ? | ❓ |
| **Super Willie** (The Head) | Spins to auto-collect all items | R2 | 7 | ? | ❓ |

---

## Environmental Items

| Official Name | Manual Description | Internal Type | Status |
|---------------|-------------------|---------------|--------|
| **Ma Bird** | Checkpoint, butt-bounce to activate | 45 (Message)? | ⏳ |
| **Green Hearts** | Shrink Klaymen | ? | ❓ |
| **Yellow Chevrons** | Restore normal height | ? | ❓ |
| **Clocks** | Temporary platforms, explode after 2 sec | ? | ❓ |
| **Treasure Balls** | Butt-bounce to reveal hidden items | ? | ❓ |
| **Teleport Balls** | End of level exits (tan=normal, red=hard) | 42-44 (Portal) | ⏳ |
| **Virtual Platforms** | Spotlight activators for temp platforms | ? | ❓ |

---

## Internal Type → Official Name Mapping

This is what we're trying to complete. Fill in as entities are visually verified.

| Internal Type | Init Function | Official Name | Category | Verified |
|---------------|---------------|---------------|----------|----------|
| 2 | Clayball_Init | **Clay** (Orange Balls) | Collectible | ⏳ |
| 5 | FlyingEnemy_Init | Flapper OR Flying Ynt Centurion? | Enemy | ❓ |
| 6 | FlyingEnemyAlt_Init | Flapper OR Flying Ynt Centurion? | Enemy | ❓ |
| 8 | ItemPickup_Init | **Halo**? | Collectible | ❓ |
| 17-23 | EnemyCluster_Init | Ground enemies (which is which?) | Enemy | ❓ |
| 25 | GroundPatrolEnemy_Init | **Phart Head** (L2) | Collectible | ⏳ |
| 26 | PathEnemy_Init | Worker Ynt? Swarm-o-Ynts? | Enemy | ❓ |
| 27 | PathEnemy_Init | Worker Ynt? Swarm-o-Ynts? | Enemy | ❓ |
| 29 | Enemy_Init | ? | Enemy | ❓ |
| 42-44 | Portal_Init | **Teleport Balls** | Interactive | ⏳ |
| 45 | MessageBox_Init | **Ma Bird** (Checkpoint) | Interactive | ⏳ |
| 61 | Sparkle_Init | Collection effect | Effect | ⏳ |
| 71 | MonkeyMageBoss_Init | **Monkey Mage** | Boss | ✅ |
| 95 | 1970sBonus_Init | **1970s Icons** | Collectible | ⏳ |
| 100 | GlennYntis_Init | **Glenn Yntis** | Boss | ✅ |
| 101 | ShrineyGuard_Init | **Shriney Guard** | Boss | ✅ |
| 102 | JoeHeadJoe_Init | **Joe-Head-Joe** | Boss | ✅ |
| 103 | Klogg_Init | **Klogg** | Boss | ✅ |

---

## Verification Workflow

1. **Extract sprite ID** from Ghidra for an internal type
2. **Find sprite images** in `evil-engine/extracted/<LEVEL>/*/sprites/`
3. **Show developer** the sprite thumbnail(s)
4. **Developer confirms** which official name matches
5. **Update this document** with ✅ status

Run the verification tool:
```bash
python3 tools/scripts/entity_sprite_report.py
# Opens: docs/reports/entity_sprites.html
```

---

## Notes

1. **Layer-Dependent Remapping**: BLB entity types remap differently per layer. Internal type 25 is Phart Head collectible on Layer 2, but may be different on Layer 1.

2. **Enemy Types 17-23**: These 7 types likely correspond to the 7+ ground enemy variants (Clay Keeper, Loud Mouth, Mental Monkey, Tempest, Jumpy, Triple Laser, El Barfo).

3. **Flying Types 5-6**: One is likely Flapper (butt-bounceable), other is Flying Ynt Centurion (weapon-only due to spikes).

4. **Level-Specific Enemies**: Some enemies only appear in specific level themes:
   - CSTL: Castle Trooper
   - FOOD: Pop-Corn Skulls, Fork Shooter Monkey
   - SNOW: Sno-Blo
   - RUNN: Super Bomber Monk
   - EVIL: Evil Engine Royal Guard
   - YNTE levels: All Ynt variants

