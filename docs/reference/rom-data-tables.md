# ROM Data Tables Reference

**Status**: 📋 Extraction Guide  
**Last Updated**: January 15, 2026  
**Purpose**: Document ROM data tables not included in C decompilation

---

## Overview

Several data tables are referenced in the decompiled C code but their actual data is in the ROM data section. This document provides extraction information for each table.

---

## Color Zone Table

**C Reference**: `DAT_8009d9c0`, `DAT_8009d9c1`, `DAT_8009d9c2`  
**RAM Address**: 0x8009d9c0  
**Size**: 60 bytes (20 RGB triplets)  
**Format**: `[R, G, B, R, G, B, ...]` (u8 values 0-255)

### Usage

**Function**: `FUN_8007ee6c` @ line 41920

```c
// Get color for trigger zone 0x15-0x28 (21-40)
u8 index = trigger_type - 0x15;  // 0-19
if (index < 20) {
    int offset = index * 3;
    R = DAT_8009d9c0[offset + 0];
    G = DAT_8009d9c1[offset + 1];
    B = DAT_8009d9c2[offset + 2];
}
```

### Extraction Method

**From ROM File** (if available):
```bash
# Calculate ROM offset: RAM address - 0x80010000
# 0x8009d9c0 - 0x80010000 = 0x8d9c0
dd if=SLES_010.90 bs=1 skip=$((0x8d9c0)) count=60 | hexdump -C
```

**From PCSX-Redux**:
```lua
local addr = 0x8009d9c0
for i = 0, 19 do
    local r = PCSX.getMemoryByte(addr + i*3 + 0)
    local g = PCSX.getMemoryByte(addr + i*3 + 1)
    local b = PCSX.getMemoryByte(addr + i*3 + 2)
    print(string.format("Color %2d (Trigger 0x%02X): RGB(%3d, %3d, %3d)", 
                        i, i+0x15, r, g, b))
end
```

### Purpose

**Collision Trigger Colors**: Triggers 0x15-0x28 apply RGB color modulation to player sprite for environmental effects (underwater tint, lava glow, etc.)

---

## Camera Acceleration Tables

### Vertical Acceleration Table

**C Reference**: `DAT_8009b074`  
**RAM Address**: 0x8009b074  
**Size**: 576 bytes (144 × s32 entries)  
**Format**: Array of 32-bit signed integers (16.16 fixed-point velocities)

**Usage**: Line 8632, 8704, 8727

```c
// Index by distance to target
index = (distance >> 1) & 0x7C;  // (distance / 2) * 4
target_velocity_y = DAT_8009b074[index];
```

**ROM Offset**: 0x8009b074 - 0x80010000 = 0x8b074

### Horizontal Acceleration Table

**C Reference**: `DAT_8009b104`  
**RAM Address**: 0x8009b104  
**Size**: 576 bytes (144 × s32 entries)

**Usage**: Line 8639, 8715

```c
index = (distance >> 1) & 0x7C;
target_velocity_x = DAT_8009b104[index];
```

**ROM Offset**: 0x8009b104 - 0x80010000 = 0x8b104

### Diagonal Acceleration Table

**C Reference**: `DAT_8009b0bc`  
**RAM Address**: 0x8009b0bc  
**Size**: 576 bytes (144 × s32 entries)

**Usage**: Line 8734

```c
index = (distance >> 1) & 0x7C;
target_velocity = DAT_8009b0bc[index];  // Combined velocity
```

**ROM Offset**: 0x8009b0bc - 0x80010000 = 0x8b0bc

### Extraction Commands

```bash
# From ROM file
dd if=SLES_010.90 bs=1 skip=$((0x8b074)) count=576 of=camera_vert_accel.bin
dd if=SLES_010.90 bs=1 skip=$((0x8b104)) count=576 of=camera_horiz_accel.bin
dd if=SLES_010.90 bs=1 skip=$((0x8b0bc)) count=576 of=camera_diag_accel.bin

# Parse as s32 array (little-endian)
python3 << 'EOF'
import struct
with open('camera_vert_accel.bin', 'rb') as f:
    data = f.read(576)
    for i in range(144):
        value = struct.unpack('<i', data[i*4:i*4+4])[0]
        pixels = value / 65536.0  # Convert 16.16 fixed to float
        print(f"[{i:3d}] Distance {i*2:3d} → Velocity {pixels:+8.4f} px/frame")
EOF
```

### Purpose

**Camera Smooth Scrolling**: Lookup tables provide ease-in/ease-out camera acceleration based on distance to target, creating professional smooth camera feel.

---

## Camera Y Offset Table

**C Reference**: `DAT_8009b038`  
**RAM Address**: 0x8009b038  
**Size**: Unknown (variable length)  
**Format**: Array of s16 values

**Usage**: Line 8789

```c
// Additional Y offset indexed by GameState[0x11A]
camera_y += DAT_8009b038[GameState[0x11A]];
```

**ROM Offset**: 0x8009b038 - 0x80010000 = 0x8b038

**Extraction**:
```bash
# Extract larger chunk to analyze
dd if=SLES_010.90 bs=1 skip=$((0x8b038)) count=256 | hexdump -C
```

**Purpose**: Camera vertical offset adjustment based on game state/mode

---

## Boss Part Position Tables

**C References**: `null_FF60h_8009b860`, `null_FFE0h_8009b862`  
**RAM Addresses**: 0x8009b860, 0x8009b862  
**Size**: 24 bytes total (6 pairs × 2 bytes each)  
**Format**: Array of s16 X/Y offset pairs

### X Offset Table

**Address**: 0x8009b860  
**Size**: 12 bytes (6 × s16)  
**Format**: Signed 16-bit X offsets

### Y Offset Table

**Address**: 0x8009b862  
**Size**: 12 bytes (6 × s16)  
**Format**: Signed 16-bit Y offsets

### Usage

**From InitBossEntity** (line 15339-15340):

```c
for (int i = 0; i < 6; i++) {
    s16 offset_x = (&null_FF60h_8009b860)[i * 2];
    s16 offset_y = (&null_FFE0h_8009b862)[i * 2];
    
    // Position boss part relative to main boss
    part->x = boss->x + offset_x;
    part->y = boss->y + offset_y;
}
```

### Extraction

```bash
# X offsets
dd if=SLES_010.90 bs=1 skip=$((0x8b860)) count=12 | hexdump -v -e '6/2 "%d " "\n"'

# Y offsets
dd if=SLES_010.90 bs=1 skip=$((0x8b862)) count=12 | hexdump -v -e '6/2 "%d " "\n"'
```

### Purpose

**Boss Structure**: Positions 6 boss part entities around main boss body (limbs, weapons, etc.)

---

## Entity Callback Table

**C Reference**: `g_EntityTypeCallbackTable`, `DAT_8009d5f8`  
**RAM Address**: 0x8009d5f8  
**Size**: 968 bytes (121 entries × 8 bytes)  
**Format**: Array of callback entries

### Entry Structure (8 bytes)

```c
struct EntityTypeEntry {
    u32 flags;       // 0xFFFF0000 = valid, 0x00000000 = unused
    u32 callback;    // Function pointer
};
```

### Status

✅ **ALREADY DOCUMENTED** in [entity-types.md](entity-types.md)

Full table with all 121 entries already extracted and documented.

---

## Extraction Priority

| Table | Size | Priority | Effort | Impact |
|-------|------|----------|--------|--------|
| **Sound IDs** | Variable | ✅ DONE | Low | High - Already extracted from calls |
| **Sprite IDs** | Variable | ✅ DONE | Low | High - 30+ extracted, more available |
| **Color Table** | 60 bytes | High | Low | Medium - Nice to have for effects |
| **Camera Tables** | 1,728 bytes | Medium | Medium | Low - Camera works without exact values |
| **Boss Positions** | 24 bytes | Medium | Low | Low - Can observe in-game |
| **Camera Y Offsets** | Unknown | Low | Medium | Low - Minor adjustment |

---

## Extraction Tools

### Required

- **PCSX-Redux**: For runtime memory dumping
- **ROM File**: SLES_010.90 executable
- **hexdump**: For binary data visualization
- **Python/C**: For parsing binary data

### Optional

- **Ghidra**: For viewing data sections
- **ImHex**: For interactive ROM browsing

---

## Status

**Sound IDs**: ✅ 35 IDs extracted (70% estimated)  
**Sprite IDs**: ✅ 30+ IDs extracted (25-30% estimated)  
**Color Table**: 📋 Location documented, needs extraction  
**Camera Tables**: 📋 Locations documented, needs extraction  
**Boss Positions**: 📋 Location documented, needs extraction

**Overall**: Quick wins (sound/sprite IDs) completed. ROM table extraction optional for polish.

---

## Related Documentation

- [Sound IDs Complete](sound-ids-complete.md) - All extracted sound IDs
- [Sprite IDs Complete](sprite-ids-complete.md) - All extracted sprite IDs
- [Entity Types](entity-types.md) - Entity callback table (already complete)
- [Physics Constants](physics-constants.md) - Other verified constants

---

**Recommendation**: Sound and sprite ID extraction provides immediate value. ROM table extraction is optional polish for exact values (estimated/observed values already documented and sufficient for implementation).

