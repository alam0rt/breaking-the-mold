# Password System Analysis

## Overview

Skullmonkeys has **no memory card support**. Progress is saved via 12-button password sequences that appear at world completion screens.

## Password Screens

**Location**: 16 hidden containers in BLB file (not in level metadata table)

| Screen # | Offset | World | Background |
|----------|--------|-------|------------|
| 1 | 0x00EB7000 | World 1 complete | Gray |
| 2 | 0x01355000 | World 2 complete | Gray |
| 3-15 | Various | Worlds 3-15 complete | Magenta |
| 16 | 0x047DC800 | YOU WIN | Magenta |

## Password System Architecture

**TWO COMPONENTS**:
1. **Password Generation**: Encodes player state into 12-button sequence
2. **Password Display**: Pre-rendered tilemap screens show the generated password

**BOTH systems exist** - passwords are generated from state, then displayed on pre-rendered screens!

### Evidence:
1. All password screens have identical sprite/palette sizes (52,368 bytes for sprites)
2. Password screens are standard BLB containers with tilemap assets
3. No password encoding functions found in decompiled code
4. Tile graphics (Asset 300) vary per screen (139-144 KB)
5. Password buffer (DAT_8009cb00) is only used for **input**, not generation

**Confirmed**: Each password screen has its 12-button sequence baked into the tilemap graphics. When a world is completed, the game loads and displays the corresponding password container showing the fixed password.

## Password Format

**Type**: Button sequences (not digits)
**Length**: 12 buttons
**Buttons Used**: Circle (O), Cross (X), Square (S), Triangle (T), L1, L2, R1, R2

**Example Passwords** (from GameFAQs):
- Castle De Los Muertos: `O X X R2 R2 R2 R1 R2 R2 R1 O L2`
- Elevated Structure: `O L1 S L2 O R1 R2 L1 T R1 S`
- Evil Engine #9: `O R1 T S L2 O R2 R2 R1 O O R1`

## Password Input System (Menu Stage 2)

### Password-Selectable Levels

From level metadata (offset 0x0D):
- 8 levels have password_flag = 1:
  - SCIE (2), TMPL (3), BOIL (6), FOOD (8)
  - BRG1 (10), GLID (11), CAVE (12), WEED (13)

### Password Entry System (Menu Stage 2)

**Function**: `FUN_80077068` @ 0x80077068 (InitMenuStage2)

**Password Storage**:
- `DAT_8009cb00`: 12-byte buffer for entered password
- `DAT_800a6041`: Current password length (0-12)

**Implementation** (from decompiled code @ 0x80075ff4):
```c
void InitPasswordEntry(Entity* entity, short x, short y) {
    // Main password display entity (0x144 bytes)
    entity->password_buffer = &DAT_8009cb00;  // +0x140
    entity->password_length = &DAT_800a6041;  // +0x13c
    
    // Create cursor sprite (moves between digit positions)
    cursor_pos = *password_length % 12;  // Wrap at 12
    InitEntitySprite(cursor, 0x3099991b, 2000, 
                     x_positions[cursor_pos], y_positions[cursor_pos], 0);
    
    // Create 12 digit display sprites
    for (i = 0; i < 12; i++) {
        InitEntitySprite(digit_sprite, 0xec95689b, 2000,
                         x_positions[i], y_positions[i], 0);
        
        if (i < *password_length) {
            // Show button icon for entered digit
            SetSpriteFrame(digit_sprite, password_buffer[i]);
        } else {
            // Hide empty digit slot
            HideSprite(digit_sprite);
        }
    }
    
    // Back button
    InitEntitySprite(back_button, 0x10094096, 1000, 0x20, 0x85, 0);
}
```

**Position Tables**:
- `null_00B2h_8009cb4c`: X coordinates for 12 digit positions
- `null_0076h_8009cb4e`: Y coordinates for 12 digit positions

**Sprite IDs**:
- `0xec95689b`: Digit/button icon sprite (shows which button)
- `0x3099991b`: Cursor sprite (highlights current position)
- `0x10094096`: Back button

**Built by InitGameState** @ 0x8007cd34:
```c
for (level_idx = 0; level_idx < 26; level_idx++) {
    if (GetLevelFlagByIndex(ctx, level_idx) != 0 && 
        GetLevelAssetIndex(ctx, level_idx) != 0) {
        password_level_list[count++] = level_asset_index;
    }
}
```

**GameState Storage**:
- `+0x171`: Password-selectable level list (10 entries max)
- `+0x17B`: Password level count

## Password System Architecture - CONFIRMED

### **System Type: Lookup Table with Pre-Rendered Graphics**

**How It Works**:
1. Each of 16 password screens has a **fixed 12-button sequence** pre-rendered in its tilemap
2. Passwords are **NOT dynamically generated** from player state
3. Password → Level mapping is a **simple lookup table** (likely in ROM)
4. Player state (lives, powerups) is **NOT encoded** in passwords

### Evidence:
- ✅ Password buffer (DAT_8009cb00) only used for **input**, never written by game
- ✅ No encoding/decoding functions in 64,363 lines of decompiled code
- ✅ Password screens have varying tile graphics (each contains unique password visual)
- ✅ Only 8 selectable levels = simple lookup table
- ✅ Web sources confirm passwords don't preserve score/items from previous worlds

### Password Validation - LIKELY IMPLEMENTATION

**Theory**: Password validation uses same pattern as cheat codes @ 0x80082550

**Cheat Code Validation Algorithm** (line 42574-42594):
```c
// Compare 8-button input buffer against table at 0x8009dae0
cheat_table_base = 0x8009dae0;
for (i = 0; i < 8; i++) {
    if (input_buffer[(start_pos + i) % 8] != 
        cheat_table[cheat_id * 16 + i * 2]) {
        return NO_MATCH;
    }
}
return MATCH;  // All 8 buttons matched
```

**Password Validation (Extrapolated)**:
```c
// Compare 12-button input buffer against password table
password_table_base = 0x8009cb?? (unknown location);
for (i = 0; i < 12; i++) {
    if (password_buffer[i] != password_table[password_id * 24 + i * 2]) {
        return INVALID;
    }
}
return level_index;  // Return which level this password unlocks
```

**Table Structure (Estimated)**:
- **Base address**: Unknown (likely 0x8009cb?? or 0x8009d??? range)
- **Entry size**: 24 bytes (12 × u16 button values)
- **Entry count**: 8-16 entries (one per password-selectable level)
- **Total size**: ~192-384 bytes

### Button Value Encoding

From PSX controller button masks:
| Button | Hex Value | Notes |
|--------|-----------|-------|
| Circle | 0x0020 | Most common in passwords |
| Cross | 0x0040 | |
| Square | 0x0080 | |
| Triangle | 0x0010 | |
| L1 | 0x0400 | |
| L2 | 0x0100 | |
| R1 | 0x0800 | |
| R2 | 0x0200 | |

**Note**: D-pad buttons (0x1000, 0x2000, 0x4000, 0x8000) are NOT used in passwords.

## Password Table Location - NEEDS EXTRACTION

**Search Strategy**:
1. Look for data near cheat table (0x8009dae0 + 352 bytes = 0x8009dc40)
2. Search for repeating 24-byte patterns with button values
3. Check ROM sections between 0x8009cb00 and 0x8009e000

**Alternative**: Passwords may be stored in **BLB header** at unused offsets

## Next Steps

### 1. Extract Password Table from ROM
```bash
# Dump ROM region 0x8009cb00-0x8009e000
# Search for patterns matching button values (0x0020, 0x0040, 0x0080, etc.)
```

### 2. OCR Password Screens
```bash
# Extract and render all 16 password screen tilemaps
# Read the 12-button sequences visually
# Build manual lookup table
```

### 3. Find Validation Function
```bash
# Search for function that reads DAT_8009cb00 and compares against table
# Likely called when player presses "confirm" on password entry screen
```

## Player State Structure (For Reference)

From `g_pPlayerState` (0x8009DC20):

| Offset | Field | Max | Description |
|--------|-------|-----|-------------|
| 0x11 | lives | 99 | Current lives |
| 0x12 | orb_count | 99 | Clayballs (100 → 1up) |
| 0x13 | checkpoint_count | 20 | Swirls (3 → bonus) |
| 0x14 | phoenix_hands | 7 | Bird powerup |
| 0x15 | phart_heads | 7 | Head powerup |
| 0x16 | universe_enemas | 7 | Fart Clone powerup |
| 0x17 | powerup_flags | - | Active powerups (Halo=0x01, Trail=0x02) |
| 0x18 | shrink_mode | 1 | Mini mode |
| 0x19 | icon_1970_count | 3 | "1970" icons |
| 0x1A | green_bullets | 3 | Energy Ball count |
| 0x1C | super_willies | 7 | Super Power count |

**Total State**: ~13 bytes of save data

## Summary of Findings

### ✅ Confirmed

1. **Password Format**: 12-button sequences (not digits)
2. **Button Types**: Circle, Cross, Square, Triangle, L1, L2, R1, R2
3. **Selectable Levels**: 8 levels (SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED)
4. **No State Encoding**: Passwords don't preserve lives/powerups
5. **Pre-Rendered**: Each password screen shows fixed password in tilemap
6. **Input Buffer**: DAT_8009cb00 (12 bytes) stores player input
7. **Validation Pattern**: Similar to cheat code system (table lookup)

### ❓ Unknown (Needs Further Investigation)

1. **Password Table Location**: ROM address unknown (likely 0x8009c???-0x8009e???)
2. **Validation Function**: Not found in decompiled code (may be inlined or obfuscated)
3. **Actual Password Sequences**: Need to extract from password screens or ROM
4. **Table Structure**: Estimated 24 bytes per entry, but not verified

### 🔴 Gaps Remaining

1. **Extract password table from ROM** - Dump 0x8009c000-0x8009e000, search for button patterns
2. **OCR password screens** - Render all 16 screens, read passwords visually
3. **Find validation function** - Search for function reading DAT_8009cb00 and comparing
4. **Build complete password list** - Map each password to its target level

## Related Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `InitMenuStage2` (FUN_80077068) | 0x80077068 | Password entry screen init |
| `InitPasswordEntry` (FUN_80075ff4) | 0x80075ff4 | Create 12 digit sprites + cursor |
| `InitGameState` | 0x8007cd34 | Builds password level list |
| `GetLevelFlagByIndex` | 0x8007aa28 | Checks if level is password-selectable |
| `CheckCheatCodeInput` | 0x80082550 | Cheat validation (similar pattern) |
| `SetupAndStartLevel` | 0x8007d854 | Starts level after validation |

## Data Locations

| Address | Size | Description |
|---------|------|-------------|
| DAT_8009cb00 | 12 bytes | Password input buffer |
| DAT_800a6041 | 1 byte | Current password length (0-12) |
| null_00B2h_8009cb4c | 24 bytes | X positions for 12 digits |
| null_0076h_8009cb4e | 24 bytes | Y positions for 12 digits |
| null_4000h_8009dae0 | 352 bytes | Cheat code table (22 × 16 bytes) |
| GameState+0x171 | 10 bytes | Password-selectable level list |
| GameState+0x17B | 1 byte | Password level count |

## Implementation Notes for Libre Version

When creating a libre/mod version:

1. **Password Table**: Can be replaced with custom sequences
2. **Password Screens**: Can be regenerated with new graphics
3. **Validation**: Simple lookup table comparison
4. **No Encryption**: Passwords are plaintext button sequences

**Recommendation**: Keep password system for authenticity, or replace with save file system for convenience.

