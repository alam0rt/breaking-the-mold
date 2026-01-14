# Documentation Gap Analysis

**Analysis Date**: January 13, 2026  
**Ghidra Functions Recognized**: 1,599  
**Documentation Files**: 38 (excluding deprecated)

## Executive Summary

The decompilation project has made excellent progress on:
- ✅ BLB file format (fully documented)
- ✅ Level loading system
- ✅ Sprite/tile rendering
- ✅ Entity system basics
- ✅ Player mechanics (recent addition)
- ✅ Entity type identification (recent addition)
- ✅ **Animation framework (5-layer system documented 2026-01-15)**

**Major gaps remain in:**
- ⚠️ Collision/physics system (can partially close from code analysis)
- ❌ Save/load/memory card system
- ❌ Boss AI and specific enemy behaviors
- ❌ Projectile/weapon system
- ⚠️ Tile attribute meanings (60-80% extractable from code)
- ⚠️ Input handling details (button mappings documented)

**See [gaps-we-can-close.md](gaps-we-can-close.md) for detailed analysis of what can be extracted from decompiled code.**

---

## Category 1: Well Documented ✅

### BLB File Format
| Area | Status | Notes |
|------|--------|-------|
| Header (0x1000 bytes) | ✅ Complete | blb/header.md |
| Level metadata | ✅ Complete | blb/level-metadata.md |
| Asset types 100-400 | ✅ Complete | blb/asset-types.md |
| Asset types 500-700 | ⚠️ Mostly | Some unknowns remain |
| TOC/sub-TOC format | ✅ Complete | blb/toc-format.md |

### Level Loading
| Area | Status | Notes |
|------|--------|-------|
| Loading state machine | ✅ Complete | systems/level-loading.md |
| LevelDataContext struct | ✅ Complete | reference/level-data-context.md |
| Asset container parsing | ✅ Complete | reference/game-functions.md |
| Sprite lookup | ✅ Complete | systems/sprites.md |

### Player System (Recently Added)
| Area | Status | Notes |
|------|--------|-------|
| Player entity structure | ✅ Complete | systems/player-system.md |
| Powerups (Halo/Trail/Shrink) | ✅ Complete | Flags at offset 0x17, 0x18 |
| Death/respawn flow | ✅ Complete | DecrementPlayerLives, RespawnAfterDeath |
| State callbacks | ✅ Documented | 4 main state callbacks |

### Entity System
| Area | Status | Notes |
|------|--------|-------|
| Asset 501 format | ✅ Complete | 24-byte entity definitions |
| Entity callback table | ✅ Complete | 121 entries at 0x8009d5f8 |
| Entity type remapping | ✅ Complete | RemapEntityTypesForLevel |
| Entity identification | ✅ Recent | systems/entity-identification.md |

---

## Category 2: Partially Documented ⚠️

### Collision System
| Area | Status | Gap |
|------|--------|-----|
| Tile collision attributes | ⚠️ Partial | Only know it exists, not what flags mean |
| Entity-to-entity collision | ⚠️ Partial | CheckEntityCollision exists but not documented |
| Trigger zones | ⚠️ Partial | CheckTriggerZoneCollision not documented |
| Wall/floor detection | ⚠️ Partial | PlayerProcessTileCollision not documented |

**Key Functions Not Documented:**
- `GetTileAttributeAtPosition` @ 0x800241f4
- `CheckEntityCollision` @ 0x800226f8
- `CheckTriggerZoneCollision` @ 0x800245bc
- `PlayerProcessTileCollision` @ 0x8005a914
- `CheckWallCollision` @ 0x80059bc8

### Audio System
| Area | Status | Gap |
|------|--------|-----|
| SPU sample format | ✅ Complete | Asset 601/602 |
| Sound effect playback | ⚠️ Partial | PlaySoundEffect @ 0x8007C388 mentioned |
| Music/streaming | ❓ Unknown | CD streaming exists but not documented |
| Sound ID table | ❓ Unknown | Many hardcoded sound IDs seen |

### Enemy Behaviors
| Area | Status | Gap |
|------|--------|-----|
| Entity type identification | ✅ Recent | Which types are which enemies |
| Ground walker AI | ❓ Unknown | func_0x8002ea3c not documented |
| Flying enemy AI | ❓ Unknown | Movement patterns unknown |
| Damage/death handling | ❓ Unknown | How enemies take damage |

---

## Category 3: Major Gaps ❌

### 1. Save/Load System
**Status**: Completely undocumented

No functions named for memory card, save, or load (except `ClearSaveSlotFlags`).

**Questions:**
- How does the password system encode progress?
- What exactly gets saved to memory card?
- How are save slots managed?

**Related functions to investigate:**
- `ClearSaveSlotFlags` @ 0x80081e84
- Menu stages 2 (password entry) and 4 (load game)

### 2. Tile Collision Attributes
**Status**: Format documented, meanings unknown

Asset 500 contains 1 byte per tile for collision attributes, but we don't know:
- Which bits mean solid/passable
- Which bits mean hazard/deadly
- How slopes/one-way platforms work
- How liquid (water/lava) is encoded

**Key function**: `GetTileAttributeAtPosition` @ 0x800241f4

### 3. Projectile/Weapon System
**Status**: Minimally documented

Klaymen can shoot projectiles but we don't know:
- How ammo count is tracked
- Projectile entity types
- Damage values
- Hitbox mechanics

**Key functions:**
- `SpawnProjectileEntity` @ 0x80070414 (exists but not documented)

### 4. Boss Fights
**Status**: Entity types known, behaviors unknown

Entity types 49, 50, 51 are boss-related but:
- Boss AI state machines not documented
- Boss attack patterns unknown
- Boss phase transitions unknown
- How boss damage is calculated

### 5. Camera System
**Status**: Exists in code, not documented

The camera follows the player and scrolls layers, but:
- Camera bounds/limits
- Camera smoothing/lookahead
- Multi-layer parallax logic

**Key function**: `UpdateCameraPosition` @ 0x80023dbc (mentioned but not documented)

### 6. FINN/RUNN Vehicle Levels
**Status**: Asset 504 documented, gameplay not

Vehicle path data (Asset 504) is extracted, but:
- How vehicle controls differ
- How auto-scrolling works
- Rail grinding mechanics

Related: `systems/player-finn.md` exists but may be incomplete

---

## Category 4: Minor Gaps

### Input System
| Missing | Notes |
|---------|-------|
| Button mappings | Which buttons do what |
| Controller vibration | How rumble is triggered |
| 2-player mode | Does it exist? |

### Menu System
| Missing | Notes |
|---------|-------|
| Menu state machine | Partially in game-functions.md |
| Password encoding | How 12-digit codes work |
| Options screen | Color picker, etc. |

### Visual Effects
| Missing | Notes |
|---------|-------|
| Palette animation | Asset 401 documented, runtime not |
| Screen transitions | Fade in/out functions |
| Weather effects | Snow, etc. |

---

## Recommendations

### High Priority (Blocking Decompilation)

1. **Tile Collision System** - Required for accurate gameplay
   - Decompile `GetTileAttributeAtPosition`
   - Document all tile attribute flags
   - Create tile collision reference

2. **Entity Init Functions** - Many are `func_0x8002XXXX`
   - Create functions for common init helpers
   - Document sprite ID tables
   - Map all enemy behaviors

### Medium Priority (Important for Completeness)

3. **Projectile System**
   - Decompile `SpawnProjectileEntity`
   - Document ammo mechanics
   - Document damage values

4. **Save/Password System**
   - Investigate password encoding
   - Document memory card format
   - Map save data structure

5. **Camera System**
   - Decompile `UpdateCameraPosition`
   - Document camera bounds
   - Document layer scrolling

### Lower Priority (Nice to Have)

6. **Boss AI** - Complex, low impact on matching
7. **Audio System Details** - Works but undocumented
8. **Visual Effects** - Cosmetic

---

## Statistics

### Ghidra Analysis
- Total functions recognized: **1,599**
- EntityType callbacks named: **~83**
- Player-related functions: **~50**
- Level loading functions: **~40**
- Remaining anonymous: **~1,400** (many are library/PSY-Q)

### Documentation Coverage
- Systems documented: 12/~20 estimated
- Reference docs: 4 files
- Analysis/unconfirmed: 3 files
- Deprecated: 5 files

### BLB Format Coverage
- Asset types documented: 16/16 (100%)
- Asset fields verified: ~90%
- Unknown fields remaining: ~10

---

## Action Items

1. [ ] Create `docs/systems/collision.md` - Document tile collision system
2. [ ] Create `docs/systems/projectiles.md` - Document weapon/ammo system  
3. [ ] Create `docs/systems/camera.md` - Document camera/scrolling
4. [ ] Create `docs/systems/save-system.md` - Document save/password
5. [ ] Update `docs/reference/game-functions.md` with collision functions
6. [ ] Create functions in Ghidra for common entity init helpers
7. [ ] Document remaining unknown fields in tile attributes
8. [ ] Cross-reference extracted sprites with entity types
