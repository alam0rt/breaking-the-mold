# Password Extraction Guide

This guide explains how to extract the complete password table from Skullmonkeys.

## Method 1: Extract from ROM Data Section

### Step 1: Dump ROM Region

```bash
# Using PCSX-Redux or PSX executable
# Dump region 0x8009c000-0x8009e000 (8 KB)

# If you have the PSX executable (SLES_010.90):
dd if=SLES_010.90 of=data_section.bin bs=1 skip=$((0x9c000)) count=$((0x2000))
```

### Step 2: Search for Password Table

```python
import struct

# Read dumped data
with open('data_section.bin', 'rb') as f:
    data = f.read()

# Button values to search for
BUTTONS = {
    0x0020: 'Circle',
    0x0040: 'Cross',
    0x0080: 'Square',
    0x0010: 'Triangle',
    0x0400: 'L1',
    0x0100: 'L2',
    0x0800: 'R1',
    0x0200: 'R2',
}

# Search for sequences of 12 button values (24 bytes)
for offset in range(0, len(data) - 24, 2):
    sequence = []
    valid = True
    
    for i in range(12):
        value = struct.unpack_from('<H', data, offset + i * 2)[0]
        if value not in BUTTONS:
            valid = False
            break
        sequence.append(value)
    
    if valid and len(sequence) == 12:
        print(f"Found password at offset 0x{offset:04x}:")
        print(' '.join([BUTTONS[v] for v in sequence]))
        print()
```

### Step 3: Verify Table Structure

Expected pattern:
```
Offset 0x000: [12 × u16] Password 1 (24 bytes)
Offset 0x018: [12 × u16] Password 2 (24 bytes)
Offset 0x030: [12 × u16] Password 3 (24 bytes)
...
Offset 0x0A8: [12 × u16] Password 8 (24 bytes)
Total: 192 bytes for 8 passwords
```

---

## Method 2: Extract from Password Screen Tilemaps

### Step 1: Extract Password Screen Containers

```bash
# Use blb_parse tool to extract password containers
./build/blb_parse /path/to/GAME.BLB --extract-passwords

# Or manually:
dd if=GAME.BLB of=password_01.bin bs=1 skip=$((0x00EB7000)) count=$((252*1024))
dd if=GAME.BLB of=password_02.bin bs=1 skip=$((0x01355000)) count=$((248*1024))
# ... etc for all 16 screens
```

### Step 2: Render Tilemaps

```python
# Use existing tilemap renderer
from blb_renderer import render_tilemap

for i in range(1, 17):
    container = load_password_container(f'password_{i:02d}.bin')
    image = render_tilemap(container)
    image.save(f'password_{i:02d}.png')
```

### Step 3: OCR or Manual Reading

```
# View rendered images
# Manually transcribe the 12-button sequences shown
# Or use OCR if button icons are recognizable
```

---

## Method 3: Runtime Memory Dump

### Step 1: Play Game to World Completion

```bash
# Use PCSX-Redux
# Play through world 1 (PHRO)
# Reach password screen
```

### Step 2: Dump Password Table from Memory

```lua
-- PCSX-Redux Lua script
function dump_password_table()
    -- Search for password table in RAM
    local search_start = 0x8009c000
    local search_end = 0x8009e000
    
    for addr = search_start, search_end, 2 do
        -- Check if this looks like a password entry
        local valid = true
        local sequence = {}
        
        for i = 0, 11 do
            local value = memory.read_u16(addr + i * 2)
            -- Check if value is a valid button
            if value ~= 0x0020 and value ~= 0x0040 and 
               value ~= 0x0080 and value ~= 0x0010 and
               value ~= 0x0100 and value ~= 0x0200 and
               value ~= 0x0400 and value ~= 0x0800 then
                valid = false
                break
            end
            table.insert(sequence, value)
        end
        
        if valid then
            print(string.format("Password found at 0x%08X:", addr))
            print_button_sequence(sequence)
        end
    end
end
```

### Step 3: Verify Against Displayed Password

```
# Compare dumped sequences with what's shown on screen
# Identify which table entry corresponds to which world
```

---

## Method 4: Extract from Ghidra

### Step 1: Load PSX Executable in Ghidra

```
File → Import File → SLES_010.90
Processor: MIPS:LE:32:default
```

### Step 2: Navigate to Data Section

```
Window → Memory Map
Find .data or .rodata section around 0x8009c000
```

### Step 3: Search for Password Table

```
Search → For Byte Patterns
Pattern: 20 00 40 00 (Circle, Cross pattern)
Or: Search for sequences of small u16 values
```

### Step 4: Define Data Structure

```c
// In Ghidra, at found location:
struct PasswordEntry {
    u16 buttons[12];
};

PasswordEntry password_table[8] @ 0x8009cb??;
```

---

## Expected Output

### Password Table Format

```c
// password_table.h
typedef struct {
    uint16_t buttons[12];
} PasswordEntry;

const PasswordEntry PASSWORD_TABLE[8] = {
    // SCIE (Science Center)
    {{0x0020, 0x0040, 0x0080, 0x0010, 0x0400, 0x0100, 
      0x0800, 0x0200, 0x0020, 0x0040, 0x0080, 0x0010}},
    
    // TMPL (Monkey Shrines)
    {{0x0020, 0x0400, 0x0080, 0x0100, 0x0020, 0x0800,
      0x0200, 0x0400, 0x0010, 0x0800, 0x0080, 0x0000}},
    
    // ... (6 more entries)
};

// Mapping to level indices
const uint8_t PASSWORD_LEVEL_MAP[8] = {
    2,   // SCIE
    3,   // TMPL
    6,   // BOIL
    8,   // FOOD
    10,  // BRG1
    11,  // GLID
    12,  // CAVE
    13,  // WEED
};
```

---

## Validation

### Test Extracted Passwords

```c
// Test program
int main() {
    uint16_t test_input[12] = {
        0x0020, 0x0040, 0x0040, 0x0200, 0x0200, 0x0200,
        0x0800, 0x0200, 0x0200, 0x0800, 0x0020, 0x0100
    };
    
    int level = ValidatePassword(test_input);
    if (level >= 0) {
        printf("Valid! Unlocks level %d\n", level);
    } else {
        printf("Invalid password\n");
    }
}
```

### Verify In-Game

```
1. Start Skullmonkeys
2. Go to Menu → Password Entry (stage 2)
3. Enter extracted password
4. Verify it loads correct level
5. Repeat for all 8 passwords
```

---

## Success Criteria

- [ ] Password table extracted (8-16 entries × 24 bytes)
- [ ] All button sequences documented
- [ ] Password → Level mapping verified
- [ ] Validation function identified or reimplemented
- [ ] Test: All passwords work in-game
- [ ] Test: Invalid passwords rejected

---

*Once complete, update docs/systems/password-system.md with final table and validation function.*

