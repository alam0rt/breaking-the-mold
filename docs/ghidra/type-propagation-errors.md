# Ghidra Type Propagation Errors

## Problem: `$$undef` Symbol Errors in Decompilation

### Symptom
When trying to decompile a function (e.g., `main()`), Ghidra fails with an error like:
```
Symbol $$undef0000005e extends beyond the end of the address space
```

The `$$undef` prefix followed by a hex value (like `0000005e` = 94 decimal) indicates Ghidra is trying to create a reference to an invalid/low memory address.

### Root Cause
This happens when:
1. A **struct field is typed as `pointer`** but can sometimes hold **small integer values** (indices, offsets, flags)
2. Ghidra's type propagation traces through code paths and assumes all values reaching an indirect call (`jalr`) must be valid code pointers
3. When a small integer (like `0x5e`) flows into the pointer-typed field, Ghidra tries to resolve it as an address

### Example: GameState.mode_callback_ptr

In Skullmonkeys, `GameState` offset +4 (`mode_callback_ptr`) is used in two ways:
```c
if (mode_callback_index < 1) {
    // Path A: Direct function pointer
    callback = *(code **)(gamestate + 4);  // mode_callback_ptr IS a pointer
}
else {
    // Path B: Table offset
    table_addr = (int)gamestate + *(int *)(gamestate + 4);  // mode_callback_ptr is an OFFSET
    callback = *(code **)(table_addr + index * 8 - 4);
}
(*callback)(...);  // jalr instruction
```

When `mode_callback_ptr` was typed as `pointer`, Ghidra traced Path A and tried to dereference address `0x5e` when the field contained an offset value.

### Solution

**Option 1: Change problematic pointer fields to `int` or `dword`**
```
Use mcp_ghidra_structs_update_field:
- struct_name: "GameState"  
- field_offset: 4
- new_type: "int"
```

**Option 2: Delete and recreate the struct** (if type propagation is cached)
```
mcp_ghidra_structs_delete(name="GameState")
# Then recreate with correct field types
```

**Option 3: In Ghidra GUI**
1. Navigate to the problematic `jalr` instruction
2. Right-click → References → Delete any references to low addresses
3. Or: Right-click → Override Signature to mark as indirect call

### Prevention

When defining structs, consider whether a field can hold both pointers AND integers:
- **Use `int`/`dword`** for fields that serve dual purposes (pointer OR offset/index)
- **Use `pointer`** only for fields that are ALWAYS valid memory addresses
- Document ambiguous fields with comments explaining their dual nature

### Affected Structs in This Project

| Struct | Offset | Field | Original Type | Fixed Type | Reason |
|--------|--------|-------|---------------|------------|--------|
| GameState | 4 | mode_callback_ptr | pointer | int | Can be callback table offset |
| GameState | 8 | unknown08 | pointer | int | Unknown dual-purpose field |
| LevelDataContext | 100 | sector_read_callback | pointer | int | Stored callback pointer; use care when the context is passed as raw words |

### Related Functions
- `main()` at `0x800828b0` - Uses GameState callback dispatch
- `jalr a1` at `0x80082b20` - The indirect call that triggers the error

---
*Documented: 2026-01-17 after resolving main() decompilation failure*
