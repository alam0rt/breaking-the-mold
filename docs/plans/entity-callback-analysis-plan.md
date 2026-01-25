# Entity Callback Analysis Plan

**Goal**: Systematically analyze and document all unknown entity callback functions, mapping them to official enemy names from the game manual with **visual sprite verification**.

**Date**: January 24, 2026

**Reference**: `docs/reference/official-enemy-names.md` - Official names from game manual

---

## ⚠️ VERIFICATION REQUIREMENT

**No entity can be marked as "verified" until:**
1. The sprite ID has been extracted from Ghidra
2. The sprite images have been shown to the developer from `evil-engine/extracted/`
3. The developer confirms the visual matches the expected entity

**Visual Verification Tool**: `tools/scripts/entity_sprite_report.py`
- Generates HTML report with embedded sprite thumbnails
- Shows which levels each sprite appears in
- Allows batch verification of entity → sprite mappings

**Run verification report:**
```bash
python3 tools/scripts/entity_sprite_report.py
# Opens: docs/reports/entity_sprites.html
```

---

## Current State

### Coverage Summary

| Category | Identified | Total | Coverage |
|----------|-----------|-------|----------|
| Unique Callbacks | ~35 | 82 | ~43% |
| Entity Types | ~45 | 121 | ~37% |
| Sprite IDs | ~40 | ~100+ | ~40% |
| **Visually Verified** | 18 | 121 | **~15%** |

### Verification Status Legend

- ✅ **Verified**: Sprite visually confirmed by developer
- ⏳ **Pending**: Sprite ID known, awaiting visual confirmation
- ❓ **Unknown**: Sprite ID not yet extracted from Ghidra

### Already Identified Entity Types

| Type | Callback | Name | Sprite ID | Levels | Status |
|------|----------|------|-----------|--------|--------|
| 0, 3, 4 | 0x8007efd0 | Pickup/Collectible | ? | ? | ❓ |
| 2 | 0x80080328 | Green Bullets | 0xb8700ca1 | SCIE+ | ✅ |
| 5 | 0x8007f7b0 | **Moving Platform A** | 0x88783718 | Various | ✅ |
| 6 | 0x8007f830 | **Moving Platform B** | 0x8818a018 | Various | ✅ |
| 7 | 0x80080408 | **Clayball** (Orange Balls) | Random | All levels | ✅ |
| 8 | 0x80081504 | **Klogg Catchable Ball** | 0x0c34aa22 | KLOG only | ✅ |
| 10 | 0x8007f244 | **Egg-Beater** | g_SpriteList | CLOU, CRYS, SOAR | ✅ |
| 11 | 0x80080478 | **Extra Life** (Klaymen's Head) | 0xa9240484 | Various | ✅ |
| 12 | 0x8007f8b0 | **Moving Platform C** | 0x9299c307 | MOSS | ✅ |
| 19 | 0x8007fa30 | **Clock Platform A** | 0x93043811 | Various | ✅ |
| 20 | 0x8007faac | **Clock Platform B** | 0xd2801814 | Various | ✅ |
| 21 | 0x8007fb28 | **Clock Platform C** | 0x12800031 | Various | ✅ |
| 23 | 0x80080558 | **Phoenix Hand** (The Bird) | 0x9158a0f6 | Various | ✅ |
| 24 | 0x8007f460 | SpecialAmmo | ? | ? | ❓ |
| 25 | 0x800805c8 | **Phart Head** | 0x8c510186 | SCIE | ✅ |
| 26 | 0x8007f2cc | **Flapper** | ? | CLOU | ✅ |
| 27 | 0x8007f354 | **Flapper Variant** | ? | CLOU | ✅ |
| 28 | 0x80080638 | PlatformA | ? | ? | ❓ |
| 29 | 0x800806a8 | **Ground Patrol Enemy** | ? | ? | ❓ |
| 42-44 | 0x80080ddc | Portal | 0xb01c25f0 | ? | ⏳ |
| 45 | 0x80080f1c | Message Box | 0xa89d0ad0 | ? | ⏳ |
| 48 | 0x80080e4c | PlatformB | ? | ? | ❓ |
| 61 | 0x80080718 | Sparkle | 0x6a351094 | SCIE | ⏳ |
| 71 | 0x80047fb8 | **Monkey Mage** (Boss) | ? | WIZZ | ✅ |
| 100 | 0x80049a54 | **Glenn Yntis** (Boss) | ? | GLEN | ✅ |
| 101 | 0x8004af64 | **Shriney Guard** (Boss) | ? | MEGA | ✅ |
| 102 | 0x8004c0e0 | **Joe-Head-Joe** (Boss) | ? | HEAD | ✅ |
| 103 | 0x8004d88c | **Klogg** (Boss) | ? | KLOG | ✅ |

### Unknown Entity Types - Need Visual Verification

Based on official names from `docs/reference/manual.md`, these need sprite verification:

#### Ground Enemies (Internal Types 17-23?)

| Official Name | Manual Description | Distinguishing Feature |
|---------------|-------------------|------------------------|
| **Clay Keeper** | Sit still or lumber, snacking on clay | Drops free clay ball when defeated |
| **Loud Mouth** | Run around flailing arms, long reach | Flailing arm animation |
| **Mental Monkey** | Faster Loud Mouth, double speed | Same as Loud Mouth but faster |
| **Tempest Pulsating Monkey** | Glow periodically, fry if bounced while glowing | Glowing/electricity effect |
| **Jumpy the Gorilla** | Large monkey that jumps constantly | Larger sprite, constant jumping |
| **Triple Laser Butt Bounce Monkey** | Leave 3 laser balls on death | Death spawns laser balls |
| **El Barfo** | Barf themselves out of own skin | Unique gross animation |

#### Flying Enemies (Internal Types 5-6?)

| Official Name | Manual Description | Key Visual |
|---------------|-------------------|------------|
| **Flapper** | Fly using hand fans, regenerate quickly | Fan sprite, butt-bounceable |
| **Flying Ynt Centurion** | Giant spikes on back | Spikes visible, weapon-only |

#### Projectile/Special Enemies

| Official Name | Manual Description | Expected Levels |
|---------------|-------------------|-----------------|
| **Robot Hover Monkey** | Indestructible, drop laser balls | Early levels |
| **Head Shooter Monkey** | Launch own heads that bite | ? |
| **Fork Shooter Monkey** | Giant fork launchers | FOOD |
| **Sno-Blo** | Shoot icy projectiles | SNOW |
| **Egg-Beater** | Razor propellers on head | ? |
| **Castle Trooper** | Stained glass guards with spears | CSTL |
| **Pipe-Cleaners** | Big-mouthed worms in pipes | ? |

#### Ynts (Bug Enemies)

| Official Name | Manual Description | Expected Levels |
|---------------|-------------------|-----------------|
| **Worker Ynt** | Scuttling bugs that eat slow things | YNTE levels |
| **Swarm-o-Ynts** | Packs of baby Ynts | YNTE levels |

---

## Analysis Strategy

### Phase 1: Static Analysis (Ghidra MCP)

For each unknown callback function:

1. **Decompile the function**
   ```
   mcp_ghidra_functions_decompile(address="0x800XXXXX")
   ```

2. **Extract key information:**
   - Sprite ID constant passed to `SetEntitySpriteId` or `InitEntityWithSprite`
   - Entity struct size (from `AllocateFromHeap` call)
   - Collision flags/masks
   - Which init helper function is used (determines behavior category)
   - State machine callbacks set at entity+0x18

3. **Categorize by init helper pattern:**

   | Helper Function | Category | Example Types |
   |-----------------|----------|---------------|
   | `CreateCollectibleWithFlags` | Collectible | 0, 3, 4 |
   | `InitGenericSpriteEntity` | Animated object | Many |
   | `InitPathFollowingEntity` | Path-based | 45 (message) |
   | Enemy-specific init | Enemy | 26, 27, 29 |
   | Effect init | Particle/Effect | 61 |

4. **Build call graph:**
   ```
   mcp_ghidra_analysis_get_callgraph(address="0x800XXXXX", max_depth=3)
   ```

### Phase 2: Cross-Reference Official Enemy Names

For each unidentified enemy type found in Ghidra:

1. **Extract behavioral characteristics from decompiled code:**
   - Movement pattern (patrol, jump, fly, static)
   - Attack/damage behavior
   - Vulnerability flags (butt-bounce, weapon-only, invulnerable)
   - Visual effect triggers (glow, electricity, fire)

2. **Match to official enemy descriptions:**
   - Compare behavior to manual descriptions
   - Note any special weaknesses mentioned

3. **Ask clarifying questions** to confirm mapping (see Phase 3)

### Phase 3: Clarifying Questions for Developer

Instead of runtime tracing, I will present findings and ask you to confirm:

> **Example Question Format:**
> 
> I found Internal Type 17 has these characteristics:
> - Uses sprite table at 0x8009XXXX
> - Movement: Horizontal patrol with occasional jumping
> - Tick callback shows gravity + velocity updates
> - No special invulnerability flags
> 
> **Could this be "Jumpy the Gorilla" (large monkey that constantly jumps)?**
> Or does the sprite look more like "Loud Mouth" (monkey flailing arms)?

### Phase 4: Cross-Reference with BLB Data

1. **Entity frequency analysis**
   - Run analysis across all extracted levels
   - Which types appear in which level themes?
   - Frequency correlates with importance

2. **Layer analysis**
   - Layer 1 = foreground (remapped types)
   - Layer 2 = gameplay entities
   - Layer 3 = special/boss

3. **Level theme correlation**
   - SCIE (Science) levels: What enemies appear?
   - CSTL (Castle) levels: Castle Troopers expected
   - HOTD (Hot Dog Factory): Fork Shooter Monkeys expected
   - YNTE (Ynt levels): Ynts and Ynt Centurions expected

---

## Priority Analysis Order

### Batch 1: Enemy Types - Match to Official Names (Est. 2-3 hours)

These need to be matched to official enemy names from the manual:

| Priority | Type | Callback | Manual Candidates | Key Question |
|----------|------|----------|-------------------|--------------|
| 1 | 17-23 | Various | Loud Mouth, Mental Monkey, Tempest, Jumpy | What sprites/behavior distinguish these? |
| 2 | 5, 6 | 0x8007f7b0, 0x8007f830 | Flapper, Flying Ynt Centurion | Which has spikes (no butt-bounce)? |
| 3 | 26, 27 | 0x8007f2cc, 0x8007f354 | Worker Ynt, Swarm-o-Ynts | Path-following vs group spawn? |
| 4 | 29 | 0x800806a8 | Clay Keeper? | Does it drop clayball on death? |
| 5 | 76, 84 | Various | Other path enemies | Level-specific variants? |

### Batch 2: Platform/Object Types (Est. 2 hours)

| Priority | Types | Callback | Question for Developer |
|----------|-------|----------|------------------------|
| 1 | 28, 47-48 | Various | Horizontal vs vertical platforms? |
| 2 | 106-108 | 0x80081014 | Elevator platforms? |
| 3 | 31-38 | Shared | Moving platforms in specific levels? |

### Batch 3: Collectibles/Effects (Est. 1.5 hours)

| Priority | Types | Callback | Official Item? |
|----------|-------|----------|----------------|
| 1 | 2 | Clayball | ✅ Already identified |
| 2 | 95 | 0x800814a4 | **1970s Icon** (SEVN unlock) |
| 3 | 61 | Sparkle | Collection effect? |
| 4 | 69-72 | Various | Decoration or collectible? |

### Batch 4: Boss Components (Est. 1 hour)

| Priority | Types | Callback | Official Boss Name |
|----------|-------|----------|-------------------|
| 1 | 71 | 0x80047fb8 | **Monkey Mage** (WIZZ) ✅ |
| 2 | 100 | 0x80049a54 | **Glenn Yntis** (GLEN) ✅ |
| 3 | 101 | 0x8004af64 | **Shriney Guard** (MEGA) ✅ |
| 4 | 102 | 0x8004c0e0 | **Joe-Head-Joe** (HEAD) ✅ |
| 5 | 103 | 0x8004d88c | **Klogg** (KLOG) ✅ |
| 6 | 49-51 | Various | Boss helper entities? |

---

## Analysis Template

For each entity type, produce findings and ask for confirmation:

```markdown
### Entity Type XX: [Proposed Name]

**Callback**: 0x800XXXXX
**Sprite ID**: 0xXXXXXXXX  
**Category**: Enemy | Collectible | Platform | Effect | Decoration
**Entity Size**: 0xXXX bytes
**Verification Status**: ❓ Unknown | ⏳ Pending | ✅ Verified

**Sprite Preview** (from evil-engine/extracted/):
> Show frame 0 of sprite from: extracted/LEVEL/stageN/sprites/sprite_0xXXXXXXXX_anim00_f00.png
> Or list path: `evil-engine/extracted/SCIE/stage0/sprites/sprite_XXXXXXXXX_anim00_f00.png`

**Found In Levels**:
- SCIE (Stage 0, 1, 2): 15 instances
- TMPL (Stage 0): 3 instances

**Behavior from Ghidra Analysis:**
- Movement: [none | patrol | path | flying | jumping]
- Collision: [none | solid | damage | collectible]
- Vulnerability: [butt-bounce | weapon-only | invulnerable]
- Special: [glows | spawns | projectile | etc.]

**Official Name Candidates:**
1. [Name A] - because [reason]
2. [Name B] - because [reason]

**Clarifying Question:**
> Please look at the sprite preview above.
> Does this match [Official Name] from the manual?
> The manual describes it as: "[description]"

**Init Helper**: function_name @ 0xXXXXXXXX

**Related Types**: [Other types with similar behavior]
```

---

## Visual Verification Workflow

### Step 1: Generate Sprite Report

```bash
cd ~/projects/btm
python3 tools/scripts/entity_sprite_report.py
```

This creates `docs/reports/entity_sprites.html` with:
- All sprites from evil-engine/extracted/ with thumbnails
- Entity type → Sprite ID mappings
- Level distribution for each sprite
- Checkboxes for batch verification

### Step 2: For Each Entity Type

1. **Extract sprite ID from Ghidra** using `mcp_ghidra_functions_decompile`
2. **Find sprite in report** by sprite ID (0xXXXXXXXX format)
3. **Present to developer** with the question:

> **Visual Verification Request - Entity Type XX**
> 
> Sprite ID: `0xXXXXXXXX`
> Preview: `evil-engine/extracted/SCIE/primary/sprites/sprite_0xXXXXXXXX_anim00_f00.png`
> 
> Found in levels: SCIE, TMPL, MOSS
> 
> Based on Ghidra analysis, this appears to be a **ground patrol enemy**.
> 
> **Does this sprite match any of these official enemies?**
> - [ ] Loud Mouth (flailing arms monkey)
> - [ ] Clay Keeper (holds clay ball)
> - [ ] Mental Monkey (faster variant)
> - [ ] Other: ___________

### Step 3: Update Status

After developer confirmation:
1. Update entity table with ✅ Verified status
2. Update `KNOWN_ENTITIES` in `entity_sprite_report.py`
3. Update `official-enemy-names.md` with confirmed mapping

---

## Clarifying Questions Template

When I analyze a callback, I'll present questions like:

### Ground Enemy Questions

> **Q1: Internal Types 17-23 (EnemyCluster)**
> These 7 types all use enemy init patterns. From the manual, ground enemies include:
> - Loud Mouth (flailing arms, geeky run)
> - Mental Monkey (faster Loud Mouth)
> - Tempest Pulsating Monkey (glows periodically)
> - Jumpy the Gorilla (large, constant jumping)
> - Clay Keeper (drops clayball on death)
>
> After I decompile each, I'll describe the behavior and ask:
> - Which sprite table does it use?
> - Does the tick callback show jumping vs patrol?
> - Any electricity/glow effect code?

### Flying Enemy Questions

> **Q2: Internal Types 5-6 (Flying Enemies)**
> From the manual, flying enemies include:
> - Flapper (hand fans, butt-bounceable)
> - Flying Ynt Centurion (spiked, can't butt-bounce)
>
> Key distinguishing factor: **Can it be butt-bounced?**
> I'll look for collision flags that prevent bounce damage.

### Special Enemy Questions

> **Q3: Weapon-Only Enemies**
> The manual lists several enemies requiring weapons:
> - Egg-Beater (propeller head)
> - Castle Trooper (spear guard)
> - Pipe-Cleaners (pipe worms)
>
> I'll search for invulnerability flags or special damage type checks.

---

## Tools & Scripts Needed

### 1. Entity Frequency Analyzer

Create Python script to analyze extracted entity data:

```bash
# Location: tools/scripts/analyze_entity_types.py
# Input: evil-engine extracted/ directory
# Output: CSV of type frequencies by level
```

### 2. Callback Batch Decompiler

Script to bulk decompile all callbacks:

```python
# For each unique callback address:
# 1. Decompile
# 2. Extract sprite ID (search for SetEntitySpriteId)
# 3. Extract struct size (search for AllocateFromHeap)
# 4. Save to analysis file
```

### 3. Level-Theme Entity Correlation

Map entity types to level codes for theme-based identification:

```python
# Which enemies appear in which level themes?
# CSTL = Castle Trooper, Fork Shooter
# HOTD = Hot Dog Factory enemies
# YNTE = Ynt enemies (Worker Ynt, Swarm-o-Ynts, Flying Ynt Centurion)
# MOSS = ?
# SCIE = ?
```

---

## Deliverables

1. **Updated entity-types.md**
   - Complete callback table with official names where known
   - Sprite ID for all 121 types

2. **Updated official-enemy-names.md**
   - Fill in all "Unknown" BLB Type mappings
   - Add Internal Type for every enemy
   - Mark all as verified or unverified

3. **Updated entity-sprite-id-mapping.md**
   - 100% sprite ID coverage
   - Visual description from manual where possible

4. **symbol_addrs.txt Updates**
   - Rename all EntityTypeXXX_InitCallback with official names
   - E.g., `EntityType017_LoudMouth_Init` if confirmed

5. **Ghidra Project Updates**
   - All callbacks renamed and commented
   - Function signatures updated

---

## Workflow Example: Analyzing Types 17-23 (Ground Enemies)

### Step 1: Decompile All Seven

```
mcp_ghidra_functions_decompile(address="0x80XXXXXX")  # For each type
```

### Step 2: Extract Distinguishing Features

For each, note:
- Sprite table address
- Movement pattern (patrol speed, jump frequency)
- Special effects (glow, electricity)
- Death behavior (drop item?)

### Step 3: Present to Developer

> **Analysis Results for Types 17-23:**
>
> | Type | Sprite | Movement | Special | Proposed Name |
> |------|--------|----------|---------|---------------|
> | 17 | 0x8009XX | Patrol, arms flail | None | Loud Mouth? |
> | 18 | 0x8009XX | Fast patrol | None | Mental Monkey? |
> | 19 | 0x8009XX | Patrol | Periodic glow | Tempest Pulsating? |
> | 20 | 0x8009XX | Constant jump | Larger size | Jumpy the Gorilla? |
> | 21 | 0x8009XX | Patrol | Drops clayball | Clay Keeper? |
> | 22 | 0x8009XX | ? | ? | ? |
> | 23 | 0x8009XX | ? | ? | ? |
>
> **Questions:**
> 1. Do these behavior descriptions match what you see in-game?
> 2. Which levels have concentrations of each type?
> 3. Any other distinguishing features I should look for?

### Step 4: Update Documentation

After developer confirmation, update all reference files.

---

## Estimated Timeline

| Phase | Tasks | Est. Time |
|-------|-------|-----------|
| Setup | Create analysis scripts | 1 hour |
| Batch 1 | Ground enemy types (17-23, 25-29) + questions | 2-3 hours |
| Batch 2 | Flying enemies (5-6) + questions | 1 hour |
| Batch 3 | Platforms (28, 47-48, 106-108) | 1.5 hours |
| Batch 4 | Effects/Collectibles | 1.5 hours |
| Review | Developer answers clarifying questions | (async) |
| Documentation | Update all reference files | 2 hours |
| **Total** | | **9-10 hours** |

---

## Success Criteria

- [ ] All 82 unique callbacks have descriptive names
- [ ] All ground enemies (Loud Mouth, Clay Keeper, etc.) mapped to internal types
- [ ] All flying enemies (Flapper, Flying Ynt Centurion, etc.) mapped
- [ ] All bosses confirmed (✅ already done: 71, 100-103)
- [ ] 90%+ sprite ID coverage
- [ ] Level distribution documented for major types
- [ ] symbol_addrs.txt fully updated with official names
- [ ] Ghidra project updated with comments

---

## Reference: Official Enemy Names Summary

From `docs/reference/official-enemy-names.md`:

### Bosses (All Confirmed ✅)
- Shriney Guard (Internal 101) - MEGA level
- Joe-Head-Joe (Internal 102) - HEAD level
- Glenn Yntis (Internal 100) - GLEN level
- Monkey Mage (Internal 71) - WIZZ level
- Klogg (Internal 103) - KLOG level

### Ground Enemies (Need Mapping)
- Loud Mouth, Clay Keeper, Mental Monkey, Tempest Pulsating Monkey, Jumpy the Gorilla, Swarm-o-Ynts

### Flying Enemies (Need Mapping)
- Flapper, Flying Ynt Centurion, Evil Engine Royal Guard, Super Bomber Monk

### Weapon-Only Enemies (Need Mapping)
- Egg-Beater, Castle Trooper, Pipe-Cleaners

### Invulnerable Hazards (Need Mapping)
- Screaming Inferno, Pop-Corn Skulls, Triple Laser Butt Bounce Monkey

---

## Notes

- **Layer-dependent remapping** is critical! BLB type 13 on L2 = Phart Head, not enemy
- Use official-enemy-names.md as authoritative source for names
- When uncertain, present options and ask developer for clarification
- Evil-engine extracted/ directory has pre-parsed entity lists for frequency analysis
- Focus on matching Ghidra behavior analysis to manual descriptions
