# PAL vs JP Binary Comparison

Comparison between PAL (SLES-01090) and JP (SLPS-01501) versions of Skullmonkeys.

## Binary Overview

| Property | PAL (SLES_010.90) | JP (slps_015.01) | Delta |
|----------|-------------------|------------------|-------|
| File Size | 618,496 bytes | 655,360 bytes | +36,864 bytes |
| RAM Base | 0x80010000 | 0x80010000 | Same |
| EXE Header | 0x800 bytes | 0x800 bytes | Same |

## Sprite IDs

**Sprite IDs are IDENTICAL between PAL and JP versions.**

Sample verified sprite IDs found in both binaries:
| Sprite ID | Description |
|-----------|-------------|
| 0x88a28194 | Icon |
| 0xb8700ca1 | Menu UI |
| 0xe2f188 | Menu item |
| 0xa9240484 | Button |

## Address Correspondence Table

### InitEntitySprite and Related Functions

| Function | PAL Address | JP Address | Delta |
|----------|-------------|------------|-------|
| InitEntitySprite | 0x8001c720 | 0x8001cb24 | +0x404 |
| memset-like init | 0x8001a0c8 | 0x8001a4cc | +0x404 |
| sprite struct init | 0x8001954c | 0x80019950 | +0x404 |
| entity setup | 0x8001c980 | 0x8001cd84 | +0x404 |
| update callback | 0x8001cb88 | 0x8001cf8c | +0x404 |
| load sprite (flag=0) | 0x8001cdac | 0x8001d1b0 | +0x404 |
| load sprite (flag=1) | 0x8001d024 | 0x8001d428 | +0x404 |
| sprite finalize | 0x8001d080 | 0x8001d484 | +0x404 |
| GPU init | 0x8007bbc0 | 0x80080888 | +0x4cc8 |

### Data Section

| Symbol | PAL Address | JP Address | Delta |
|--------|-------------|------------|-------|
| GameState | 0x800ae3e8 | 0x800af34c | +0xf64 |

## Code Section Offsets

The JP binary has code shifted by varying amounts depending on the section:

- **Near InitEntitySprite (0x8001xxxx)**: +0x404 (consistent)
- **GPU/graphics code (0x8007xxxx)**: +0x4cc8
- **Data section (0x800axxxx)**: +0xf64

The non-uniform offsets suggest the JP binary has additional code or data inserted between sections.

## Dead Code Discovery

An interesting finding: **JP contains dead code that exactly matches PAL's InitEntitySprite**.

| Address | Callers | Notes |
|---------|---------|-------|
| JP 0x8001c324 | 0 | Byte-identical to PAL 0x8001c720 (except JAL targets) |
| JP 0x8001cb24 | 160 | Active InitEntitySprite (different instruction layout) |

The dead code at JP 0x8001c324:
- Has 18/20 instruction match with PAL InitEntitySprite
- Only differences are JAL targets (which point to relocated functions)
- Has **zero callers** - completely unused

This suggests:
1. JP may have been compiled from a slightly different source version
2. Code reorganization occurred between regional builds
3. The linker included vestigial code from an earlier build

## Function Call Patterns

### PAL InitEntitySprite Callers
- 152 direct JAL calls to 0x8001c720
- Used by entity initialization functions throughout the codebase

### JP InitEntitySprite Callers  
- 160 direct JAL calls to 0x8001cb24
- Slightly more callers than PAL (8 additional call sites)

## Methodology

### Finding Corresponding Functions

1. **Pattern matching**: Search for identical instruction sequences (ignoring JAL targets)
2. **Caller analysis**: Count JAL references to candidate addresses
3. **Decompilation comparison**: Verify structure matches in Ghidra
4. **Sub-function tracing**: Follow called functions to build correspondence table

### JAL Encoding Formula
```
JAL instruction = 0x0C000000 | ((target_addr >> 2) & 0x03FFFFFF)
Target address = 0x80000000 | ((instruction & 0x03FFFFFF) << 2)
```

### Address Conversion
```
File offset = (RAM address - 0x80010000) + 0x800
RAM address = (File offset - 0x800) + 0x80010000
```

## Ghidra Instances

For analysis, two Ghidra instances were used:
- **Port 8192**: PAL (SLES_010.90)
- **Port 8193**: JP (slps_015.01)

## Summary

1. **Sprite IDs are identical** - No localization changes to sprite asset references
2. **Code layout differs** - JP has ~+0x400 offset for early code, larger offsets for later sections
3. **Dead code exists** - JP contains unused code matching PAL exactly
4. **JP is larger** - 36KB additional code/data, possibly debug info or region-specific features
5. **Same decompiled logic** - InitEntitySprite produces identical C code in both versions

## Future Work

- [ ] Map more function correspondences between PAL and JP
- [ ] Investigate what the additional 36KB in JP contains
- [ ] Check if the dead code region contains other unused PAL-matching functions
- [ ] Compare BLB file loading code between versions
