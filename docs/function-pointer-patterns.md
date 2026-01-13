# Function Pointer Patterns in PSX Games

## Why Ghidra Misses These Functions

PSX games like Skullmonkeys heavily use **function pointer tables** for entity behaviors and state machines. Ghidra's static analysis can't recognize functions that are:

1. **Never directly called** - only referenced via table lookups
2. **Stored in data sections** - look like data, not code
3. **Nested callbacks** - function A stores pointer to function B, which stores pointer to C

This was standard practice in 90s game development:
- Entity types dispatch via `callback_table[entity_type](entity)`
- Player states use `state_handlers[current_state](player)`
- No virtual functions (C, not C++) - just raw pointer arithmetic

## Address Ranges in Skullmonkeys

| Range | Purpose |
|-------|---------|
| `0x80047xxx-0x80055xxx` | Entity tick/behavior callbacks |
| `0x8005bxxx-0x80068xxx` | Player state callbacks |
| `0x8006axxx-0x80078xxx` | Entity initialization callbacks |
| `0x8009D5F8` | Main entity callback table (121 entries × 8 bytes) |

## Discovery Process

1. **Scan callback tables** for function pointers not recognized by Ghidra
2. **Create functions** at those addresses with semantic names
3. **Decompile new functions** and scan for nested `LAB_` references
4. **Iterate** until convergence (typically 3-4 passes)

### Scripts

```bash
# Find unrecognized pointers in data section
python3 scripts/find_unrecognized_functions.py --scan-data

# Find LAB_ references in newly created callbacks
python3 scripts/find_lab_references.py
```

### Results (Jan 2026)

| Pass | Functions Found |
|------|-----------------|
| Data section | 177 |
| LAB_ pass 1 | 91 |
| LAB_ pass 2 | 64 |
| LAB_ pass 3 | 21 |
| LAB_ pass 4 | 3 |
| **Total** | **442 new functions** |

## Naming Convention

- `EntityCallback_XXXXXXXX` - Entity tick/update handlers
- `PlayerCallback_XXXXXXXX` - Player state machine handlers  
- `EntityInitCallback_XXXXXXXX` - Entity spawn/initialization
- `Callback_XXXXXXXX` - Generic/unknown purpose

Once behavior is understood, rename to descriptive names (e.g., `Player_HandleJump`, `Entity_ClayballTick`).
