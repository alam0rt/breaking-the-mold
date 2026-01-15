# Decompilation Quick Reference

## One-Liner Commands

```bash
# Find candidate functions
python3 scripts/analyze_candidates.py --easy --skip-decompiled

# Decompile a function (full workflow)
python3 scripts/decompile.py FunctionName --full

# Just see what would happen
python3 scripts/decompile.py FunctionName --dry-run

# Compare Ghidra's version
python3 scripts/decompile.py FunctionName --ghidra

# Build and verify
make && make check
```

## Common Patterns

### Simple accessor (like GetLevelCount)
```c
u8 FunctionName(void *gameState) {
    return ((u8 *)(*(s32 *)((u8 *)gameState + 0xOFFSET)))[0xINDEX];
}
```

### Multiple field access
```c
void FunctionName(void *arg) {
    *(s32 *)((u8 *)arg + 0xOFF1) = *(s32 *)((u8 *)arg + 0xOFF2);
    *(u8 *)((u8 *)arg + 0xOFF3) = 1;
}
```

### Avoid local variables (they use s0/s1)
❌ Bad:
```c
s32 value = *(s32 *)((u8 *)arg + 0x1C);
DoSomething(value);
```

✓ Better:
```c
DoSomething(*(s32 *)((u8 *)arg + 0x1C));
```

## Address Conversion

```bash
# VRAM to ROM offset
./scripts/addr2offset.sh 0x8007ACDC
# Output: Offset: 0x6B4DC

# Formula: ROM = (VRAM - 0x80010000) + 0x800
```

## Debugging Non-Matching

```bash
# Compare your compiled vs expected
mipsel-unknown-linux-gnu-objdump -d build/pal/src/YourFunc.o

# Regenerate expected ASM
mv src/YourFunc.c src/YourFunc.c.bak
python3 -m splat split config/splat.pal.yaml
cat asm/pal/nonmatchings/YourFunc/YourFunc.s
mv src/YourFunc.c.bak src/YourFunc.c
```

## YAML Segment Format

```yaml
# In config/splat.pal.yaml under main -> subsegments:
- [0x6B1B0, c, GetLevelCount]      # ROM offset, type, name
- [0x6B1C4, c, StateAccessors]     # Next function
- [0x6B228, asm]                   # Assembly gap
```

## Symbol File Format

```bash
# In symbol_addrs.txt:
FunctionName = 0x8007ACDC; // size:0x14 // Comment
```

## Common Errors

**"ruamel.yaml not installed"**
```bash
pip install ruamel.yaml
```

**"Function already in 'c' segment"**
- Need to manually split existing C file
- See GetAssetCount example in CreditsAccessors.c

**"BUILD DOES NOT MATCH"**
- Check assembly with objdump
- Simplify code (remove local variables)
- Match exact operation order from original ASM

## Success Checklist

- [ ] Function in Ghidra
- [ ] Added to symbol_addrs.txt
- [ ] Segment in splat.pal.yaml
- [ ] ASM generated in asm/pal/nonmatchings/
- [ ] C file in src/
- [ ] Build succeeds
- [ ] `make check` passes ✓

## Files You'll Touch

1. `config/splat.pal.yaml` - Segment definitions
2. `symbol_addrs.txt` - Function addresses
3. `src/FunctionName.c` - Your decompiled code
4. `asm/pal/nonmatchings/FunctionName/` - Generated ASM (reference)

## Help

- Full guide: `docs/DECOMPILATION_WORKFLOW.md`
- Improvements doc: `docs/DECOMPILATION_IMPROVEMENTS.md`
- Splat wiki: https://github.com/ethteck/splat/wiki
