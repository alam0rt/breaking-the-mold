---
title: "Ancient Notes"
category: root
tags: [ancient]
---


## Compiler Investigation

Testing func_8007A62C (0x254 bytes) across different GCC versions:

| Version | Output Size | Difference |
|---------|-------------|------------|
| 2.5.7-psx | 0x254 | **EXACT MATCH** |
| 2.6.0-psx | 0x25C | +8 bytes |
| 2.6.3-psx | 0x25C | +8 bytes |
| 2.7.2-psx | 0x25C | +8 bytes |
| 2.8.0-psx | 0x25C | +8 bytes |
| 2.8.1-psx | 0x25C | +8 bytes |
| 2.91.66-psx | 0x258 | +4 bytes |
| 2.95.2-psx | 0x258 | +4 bytes |

**Conclusion: Skullmonkeys was likely compiled with GCC 2.5.7 (PSX variant)**

Note: While size matches, byte-level differences exist due to:
- Register allocation differences (original saves $s0 before other regs)
- Instruction scheduling/reordering
- Our C code may not perfectly match original source

Scripts for testing:
- `./scripts/build_old_gcc.sh` - Download/manage GCC versions
- `./scripts/find_matching_gcc.sh` - Compare compiled output sizes

---

What does this sequence mean?

```
 % strings game.blb| grep -e '^:[0-9]:' | sort | uniq -c                                                                                                                                                          :)
      6 :0:9
      2 :0::BNOJjj
      5 :1:1c\d
      2 :1:aa
      1 :3:114@@44d
      1 :3:BXXmE:0%
      3 :3:J?:@?Zffov
      1 :3:v
      4 :4:4--&"&**:DY
    885 :5:66666:6>6>V>W>W>W>WBxBxB
    110 :5:666>V>W>WBxB
      8 :5:%-6&66X
      1 :5:S
      5 :6:C:
      3 :7:7;;T
      3 :8:=2W2
      1 :8:84(
      5 :8:ED>HJ?9(-/8LMG5-5:FACICYz
      6 :9:&
     12 :9::
      6 :9:[

```

`00 00 01 00 B1 31 D2 35 F3 35 14 3A 15 3A 15 3A 15 3A 35 3A 36 36 36 36 36` appears everywhere, asset header?


888 counts of `\x00\x00\x01\x00\xB1\x31\xD2\x35\xF3\x35\x14\x3A\x15\x3A\x15\x3A\x15\x3A\x35\x3A\x36\x36\x36\x36\x36\x3A\x36\x3E\x36\x3E\x56\x3E\x57\x3E\x57\x3E\x57\x3E\x57\x42\x78\x42\x78\x42\x99\x42\x99\x46\xBB\x4A\xDC\x4E\x1D\x57\x5F\x63\x7F\x67\x7F\x6B\x9F\x6F\xBF\x73\x03`

Another pattern that appears near the above `01 00 18 00 3C 00 00 00`

```
binwalk -R '\x01\x00\x18\x00\x3C\x00\x00\x00' game.blb | wc -l                                                                                                                                                 
984

```


## Levels?

There are 26 level entries in the PAL BLB header (stored at header + 0xF31), including MENU as entry 0.
The "90 levels, 17 worlds" count likely includes sub-areas/screens within each level entry.

`>5>6>6B6B6B6B6BWBWBWFWF` appears 91 times, header for level?


## Game BLB loading

The GAME.BLB file offset is found by `CdSearchFile` and stored in `g_GameBLBSector`. This is then accessed by `CdBLB_ReadSectors(sectorOffset, numSectors, destBuffer)`. Each sector is 2048 bytes (standard CD-ROM sector size).

Interesting structures start at 0x1000 and 0x6000. The latter has different content however.

### Runtime structures touched by `LoadBLBHeader`

- `D_800A5960` always points at the live `GameState` blob (defaulting to `D_8009DC40` in `main`). The offsets we care about are:
    - `+0x3C` (`headerMirror`): pointer to `headerBuffer + 0x1000`. The engine seems to use this as a scratch page when the active BLB window advances.
    - `+0x40` (`headerBuffer`): destination for the first two sectors (`CdBLB_ReadSectors(0, 2, ...)`). Runtime tools and memdumps show this is `0x800AE3E0` (PAL) or `0x800AE450` (other versions).
    - `+0x84` (`cdRequest`): 0x80-byte control block handed to `func_8007A1BC`/`func_8007BB00`, but the same window is also parsed by `func_8007A9B0`/`getLevelName`. After a synchronous read completes, `+0x84` contains the live header contents.

### CD Request Control Block layout (the 0x80-byte struct at `GameState+0x84`)

These offsets are relative to `cdRequest` (i.e., `GameState+0x84`):

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| +0x00 | 0x58 | (scratch) | Cleared by `func_8007A234` – used as working area |
| +0x58 | 4 | (unused?) | Read by `func_8007BAE4` |
| +0x5C | 4 | headerPtr | Pointer to BLB header buffer (set by `func_8007A1BC`) |
| +0x60 | 1 | state | Initialised to `0xFF` by `func_8007A1BC` |
| +0x64 | 4 | callback | Completion callback (e.g., `func_80020848`) |
| +0x68 | 0x18 | (zeroed) | Cleared by `func_8007A218` |

`func_8007BB00(flag, cdRequest)` simply stores the flag in sdata global `D_800A6060` and the cdRequest pointer in `D_800A6064`, allowing background systems to poll / invoke the callback later.
    - `+0x11C` (`headerBufferSize`): length of the allocation reserved for header streaming (initialised to `0x10000`).
    - `+0x12C` (`headerMagic`): set to `0x01234567` and mirrored to the BSS word `D_801FC400`, providing a simple "header is ready" sentinel for other subsystems.
- `D_800A5954` points at a 0xA650-byte engine context block (`D_800907EC`). Offset `+0xA650` inside that block is a `void *` pointing at the persistent BLB header buffer in main RAM; this is now expressed as `engine->blbHeaderBufferBase` in `LoadBLBHeader`.
- `D_801FC400` is a standalone word in the high BSS. `LoadBLBHeader` writes the same `0x01234567` magic there immediately after populating `GameState+0x12C`, which is how other high-level systems detect whether the BLB header contents in RAM are valid.

The `main` loop’s hot path repeatedly dereferences `D_800A5960 + 0x84` to enumerate level assets (`func_8007A9B0`, `getLevelName`, `func_8007ACF0`, etc.), confirming that everything relying on BLB metadata shares a single in-memory window managed by `LoadBLBHeader`.


## Memory addresses

Following CdReads to track loads from game.blb

```
0x800AE450 gets the first chunk of gameblb data (starting at 0x0)
0x80116450 gets the second chunk which starts at 0x6000 in blb (could be intro movie sequencing?)
0x80116450 then gets overwritten with data from 0xb000 from blb
0x80116450 then gets overwritten by 0xd000  from blb (and intro credits play)
0x80116450 gets overwritten by data from 0x62800
0x800af450 gets data from blb at 0x64800 (about 64792 bytes worth)
0x800bc4b4 gets loaded with data from blb at 0x74800 (to 0x95fff) (the bytes at this location appears quite a bit - probably an asset header!)
0x800bc4b4 gets overwrriten with data from 0x96000 to 0x189fff) menu now loads

/* Presses start on Start Game */
0x80116450 is loaded with data at 0x643000 to 0x6477ff (science centre)
0x800af450 loaded at 0x647800 to 0x6e67ff
0x8012f42c loaded at 0x6e6800 (to perhaps 0x746bf4ish)
0x8012f42c loaded from 0x746bf4

/* Monkey Shrines */
0x80116450 loads 0x9c524f to 0x9c9f87 // might contain load screen image?
0x800af450 loads 0x9ca252 to 
```

### Notes on memory location 0x8009DD20

`0x8009DD20` = `D_8009DC40` + 0xE0, i.e., GameState + 0xE0

The region from `0x8009DD20` to `+0x58` looks like where the level info is loaded.
The pointer is saved and 0xA is added to it afterwards.

```
0x80110450 is interesting and gets loaded with lots of 80 00 80 00s
0x800e8450 seems to be the buffer for the DecDCTIn function on start up
0x800af450 appears to be the VLC table and is about 0x800 bytes long
0x80116450 is the compressed MDEC frames
```
## Load screens
load_blb_asset sector_offset / sector_count values:

- `0xC / 0xA` == Skullmonkeys title card (sector 12, 10 sectors = byte offset 0x6000, size 0x5000)
- `0xC5 / 0x4` == Loading screen (sector 197, 4 sectors = byte offset 0x18A00, size 0x2000)
- `0xC86 / 0x9` == Science centre (sector 3206, 9 sectors = byte offset 0x190C00, size 0x4800)

## Interesting bytes

```
08 00 00 00 64 00 00 00 24 00 00 00 64 00 00 00 seems to be a header for an asset (appears 74 times)
0B 00 00 00 64 00 00 00 24 is another
03 00 00 00 58 02 another asset?

DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
411648        0x64800         Raw signature (\x03\x00\x00\x00\x58\x02)
4265984       0x411800        Raw signature (\x03\x00\x00\x00\x58\x02)
6584320       0x647800        Raw signature (\x03\x00\x00\x00\x58\x02)
10287104      0x9CF800        Raw signature (\x03\x00\x00\x00\x58\x02)
13416448      0xCCB800        Raw signature (\x03\x00\x00\x00\x58\x02)
14094336      0xD71000        Raw signature (\x03\x00\x00\x00\x58\x02)
15687680      0xEF6000        Raw signature (\x03\x00\x00\x00\x58\x02)
20525056      0x1393000       Raw signature (\x03\x00\x00\x00\x58\x02)
24625152      0x177C000       Raw signature (\x03\x00\x00\x00\x58\x02)
26855424      0x199C800       Raw signature (\x03\x00\x00\x00\x58\x02)
28549120      0x1B3A000       Raw signature (\x03\x00\x00\x00\x58\x02)
32604160      0x1F18000       Raw signature (\x03\x00\x00\x00\x58\x02)
36532224      0x22D7000       Raw signature (\x03\x00\x00\x00\x58\x02)
39757824      0x25EA800       Raw signature (\x03\x00\x00\x00\x58\x02)
42727424      0x28BF800       Raw signature (\x03\x00\x00\x00\x58\x02)
46336000      0x2C30800       Raw signature (\x03\x00\x00\x00\x58\x02)
47554560      0x2D5A000       Raw signature (\x03\x00\x00\x00\x58\x02)
51484672      0x3119800       Raw signature (\x03\x00\x00\x00\x58\x02)
53563392      0x3315000       Raw signature (\x03\x00\x00\x00\x58\x02)
56328192      0x35B8000       Raw signature (\x03\x00\x00\x00\x58\x02)
60542976      0x39BD000       Raw signature (\x03\x00\x00\x00\x58\x02)
64204800      0x3D3B000       Raw signature (\x03\x00\x00\x00\x58\x02)
65869824      0x3ED1800       Raw signature (\x03\x00\x00\x00\x58\x02)
67350528      0x403B000       Raw signature (\x03\x00\x00\x00\x58\x02)
71194624      0x43E5800       Raw signature (\x03\x00\x00\x00\x58\x02)
72243200      0x44E5800       Raw signature (\x03\x00\x00\x00\x58\x02)
```

## Decode function

Before loading the BLB asset, two functions are called to work out the offset and the number of sectors to read. Both of these functions take in a pointer to the GameState structure (at `D_8009DC40`). The pointer + 0x5C points to `headerMirror` (header + 0x1000), and the code reads the state byte at `header + 0xF37`.

(Note: The original notes referenced address `0x8009DD20`, which is `D_8009DC40 + 0xE0` (GameState + 0xE0). The `+0x5C` offset may have been relative to a different structure.)

### BLB Header Base Addresses (version dependent)
| Version | Header Base | Offset Table (base + 0x10) |
|---------|-------------|---------------------------|
| SLES_010.90 (PAL) | `0x800AE3E0` | `0x800AE3F0` |
| Other versions | `0x800AE450` | `0x800AE460` |

The 0x70 byte difference may be due to different executable sizes or memory layout between regions.

### Offset Calculation Logic

```
state = *(header + 0xF37)   // Current game state byte

if (state == 0x2):
    // Complex calculation for state 2
    index = *(header + 0xF92) + 1
    offset = *(header + 0xF18 + (index * 0xC))
    
else if (state < 1):
    offset = 0x0
    
else if (state >= 0x3 && state < 0x6):
    // Level loading path - uses the offset table at ~0xCC0
    level_index = *(header + 0xF93)           // e.g., 0x1 for PIRA
    entry_ptr = offset_table + (level_index * 0x10) // 0x10 byte entries in offset table
    offset = *(u16*)(entry_ptr + 0xC)         // Sector offset
    sectors = *(u16*)(entry_ptr + 0xE)        // Sector count
```

### BLB Header Structure Overview

The BLB header (first 0x1000 bytes) contains two main structures:

1. **Level Metadata Entries** (at offset 0x0000, 0x70 bytes each)
   - Contains level name, display info, etc.
   - Level name is at entry + 0x5B (null-terminated string)
   - Level count stored at header + 0xF31

2. **Sector Offset Table** (at offset ~0xCC0, 0x10 bytes each)
   - Contains BLB sector offset/count for loading level data
   - Used when actually loading level assets from disc

### Level Metadata Entry Format (0x70 bytes each, starting at offset 0x0000)
```
struct LevelMetadataEntry {
    u8  data[0x5B];       // +0x00: Various level metadata (TBD)
    char name[21];        // +0x5B: Level name, null-terminated (e.g., "Options", "Skullmonkey Gate")
};
```

Accessed by `getLevelName(arg0, levelIndex)` which returns `*(arg0+0x5C) + (levelIndex * 0x70) + 0x5B`.

### Sector Offset Table Entry Format (0x10 bytes each, starting at ~0xCC0)
```
struct SectorTableEntry {
    u16 unknown0;         // +0x00: Unknown (possibly flags/type)
    u8  unknown2;         // +0x02: Unknown
    char code[5];         // +0x03: 4-char level code + null (e.g., "PIRA", "MENU")
    char shortName[4];    // +0x08: Short description
    u16 sector_offset;    // +0x0C: Starting sector in BLB
    u16 sector_count;     // +0x0E: Number of sectors to read
};
```

**Example - PIRA level (from offset table at 0xCD0):**
- Entry: `37 05 0a 50 49 52 41 00 44 6f 6e 00 0c 00 0a 00`
- Code: "PIRA"
- Sector offset (at +0x0C): `0x000C`
- Sector count (at +0x0E): `0x000A`
- Byte offset in BLB: `0xC * 2048 = 0x6000`
- Bytes to read: `0xA * 2048 = 0x5000`

**Example - MENU level (from offset table at 0xD00):**
- Entry: `00 00 00 4d 45 4e 55 00 4f 70 74 00 19 08 0a 00`
- Code: "MENU"
- Sector offset: `0x0819`
- Sector count: `0x000A`
- Byte offset in BLB: `0x819 * 2048 = 0x40C800`

The offset `0x8009DD80` is initialised with `0xFF`, then it is overflowed to `0x00`... increases by 1 every time to `0x5`, then skips to `0x9`...
	
	
## 0x8009DD80 (GameState + 0x140)
This memory location seems to define where to jump to in the BLB header (game state machine). For example, 

## Releases

Demo disk http://redump.org/disc/68357/



```
4D 45 4E 55 00 4F 70 74 69 6F 6E 73 00 00 00 00 00 00 00 00 00 00 00 00 00 00 F8 07 3F 01 1C 9F 0E 00 C4 FE 07 00 01 00 03 00 96 66 A0 60 71 43 00 00 00 00 00 00 00 00 37 09 A6 0A F0 0B 00 00 00 00 00 00 00 00 D4 00 C7 00 5D 00 00 00 00 00 00 00 00 00 0B 0A 6D 0B 4D 0C 00 00 00 00 00 00 00 00 9B 00 83 00 0E 00 00 00 00 00 00 00 00 00

```

Gameblb loading

- The first 2 sectors (4096 bytes / 0x1000) is loaded into the global buffer (offset +0x40)
- A buffer that is used a lot is at 8011650

Mystery solved:

The header of the file points to the various data structures:
	PIRA has two values (at 0xCDC, the offset, and 0xCDE, size) of 0xC and 0xA respectively.
	We can find the offset in the BLB by taking the offset and multiplying it by 2048 (psx sector size).
	
	0xC * 2048 = 0x6000
	0xA * 2048 = 0x5000
	

--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/beta.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	c020003802000200200080020228288000020220288080020228200080020228
[0xa800]	c010003801000200200080020228288000020220288080020228200080020228
[0x62000]	c010003801000200200080020228288000020220288080020228200080020228
[0xcc6800]	402c003804000200200080020228288000020220288080020228200080020228
[0x481e000]	c010003801000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/papx-90053.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	c028003804000200200080020228288000020220288080020228200080020228
[0xa800]	c010003801000200200080020228288000020220288080020228200080020228
[0xce9000]	402c003804000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/sles-01090.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	002d003804000200200080020228288000020220288080020228200080020228
[0xb000]	c010003801000200200080020228288000020220288080020228200080020228
[0x62800]	c010003801000200200080020228288000020220288080020228200080020228
[0xcc7000]	402c003804000200200080020228288000020220288080020228200080020228
[0x481e800]	c010003801000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/sles-01091.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	402c003805000200200080020228288000020220288080020228200080020228
[0xa800]	0012003801000200200080020228288000020220288080020228200080020228
[0x6d000]	0012003801000200200080020228288000020220288080020228200080020228
[0xcdd800]	0031003803000200200080020228288000020220288080020228200080020228
[0x4842000]	0012003801000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/sles-01092.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	002f003804000200200080020228288000020220288080020228200080020228
[0xb000]	4013003801000200200080020228288000020220288080020228200080020228
[0x6a800]	4013003801000200200080020228288000020220288080020228200080020228
[0xcdc800]	e02b003804000200200080020228288000020220288080020228200080020228
[0x483c000]	4013003801000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/slps-01501.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	c028003804000200200080020228288000020220288080020228200080020228
[0xa800]	c010003801000200200080020228288000020220288080020228200080020228
[0xce9000]	402c003804000200200080020228288000020220288080020228200080020228
[0x4ae1800]	c010003801000200200080020228288000020220288080020228200080020228
--------------------------------------------------------------------------------
File: /mnt/share/sam/skullmonkeys/disc/blbs/slus-00601.blb

[0x1000]	a033003807000200200080020228288000020220288080020228200080020228
[0x6000]	002d003804000200200080020228288000020220288080020228200080020228
[0xb000]	c010003801000200200080020228288000020220288080020228200080020228
[0x62800]	c010003801000200200080020228288000020220288080020228200080020228
[0xcc7000]	402c003804000200200080020228288000020220288080020228200080020228
[0x481e800]	c010003801000200200080020228288000020220288080020228200080020228

## BLB header analysis

Use Python to parse the first 0x1000 bytes of a BLB file and dump the entry table:

```python
import struct
with open('disks/blb/GAME.BLB', 'rb') as f:
    data = f.read(0x1000)
    # Offset table starts at ~0xCC0, each entry is 0x10 bytes
    for i in range(26):  # 26 levels
        entry = data[0xCC0 + i*0x10 : 0xCC0 + (i+1)*0x10]
        code = entry[3:7].decode('ascii', errors='replace').rstrip('\x00')
        sector_off = struct.unpack('<H', entry[0xC:0xE])[0]
        sector_cnt = struct.unpack('<H', entry[0xE:0x10])[0]
        print(f"{i:2d}: {code:5s} sector_off=0x{sector_off:04X} ({sector_off*2048:08X})")
```

- Memory snapshots (for cross-checking against the live disc image):

    | File | Size | SHA1 |
    |------|------|------|
    | `extracted_memory/game_blb_800AE3E0.bin` | 0x152010 | `d06b822d601007f7ba9a789848bec24b04635a20` |
    | `extracted_memory/bss_800A6168.bin` | 0x8288 | `97ee0cafaf977bb99d980ad2fb381d8657200c5e` |
    | `extracted_memory/sbss_800A6120.bin` | 0x48 | `de6fd14c9444f637c15b4aa780552df465b3333d` |

    These dumps look consistent with the in-game header contents (state byte at
    `+0xF37` currently reads `0x05`), but we are preferring the disc-resident
    `GAME.BLB` going forward.


## Runtime BLB Header Access Trace

Captured using `scripts/trace_blb_header.lua` with PCSX-Redux. This documents which
functions access which parts of the BLB header during actual gameplay.

### State/Index Sliding Window Pattern

The BLB header region `0xF34-0xFFF` contains runtime state used by accessor functions.
The accessor functions use a **sliding window pattern** where paired state/index bytes
are read at offsets determined by an internal counter:

| Offset | State Byte | Index Byte | Used By |
|--------|------------|------------|---------|
| 0 | 0xF36 | 0xF92 | func_8007ABCC, func_8007AC54 (initial credits) |
| 1 | 0xF37 | 0xF93 | func_8007ABCC, func_8007AC54 (after advance) |
| 2 | 0xF38 | 0xF94 | GetMovieUnknown00, GetMovieSectorCount, GetMovieFilename (movie 0) |
| 3 | 0xF39 | 0xF95 | Movie accessors (movie 1) |
| 4 | 0xF3A | 0xF96 | Movie accessors (movie 2) |
| 5 | 0xF3B | 0xF97 | Movie accessors (movie 3) |
| ... | ... | ... | ... |
| 9 | 0xF3F | 0xF9B | Credits (before menu) |
| 12 | 0xF40 | 0xF9C | func_8007A62C (LevelDataParser, level mode) |

### Key Observations

- **State byte at 0xF36+n** determines mode (1=movie, 2=credits, 3=level, etc.)
- **Index byte at 0xF92+n** selects entry within the relevant table
- Movie accessors use n=2 (0xF38, 0xF94) and advance with each movie
- Level parser uses n=12 (0xF40, 0xF9C) when loading menu/level 0
- Credits accessors advance their window offset between calls

### Function Access Summary

| Function | Address | First Read | Second Read | Table Access |
|----------|---------|------------|-------------|--------------|
| func_8007ABCC | 0x8007ABCC | 0xF36+n (mode) | 0xF92+n (index) | Credits table |
| func_8007AC54 | 0x8007AC54 | 0xF36+n (mode) | 0xF92+n (index) | Credits table |
| GetMovieFilename | 0x8007ADC8 | 0xF38+n (mode) | 0xF94+n (index) | Movie table |
| GetMovieUnknown00 | 0x8007AE14 | 0xF38+n (mode) | 0xF94+n (index) | Movie[n].reserved @ 0xB60 |
| GetMovieSectorCount | 0x8007AE58 | 0xF38+n (mode) | 0xF94+n (index) | Movie[n].sector_count @ 0xB62 |
| func_8007A62C | 0x8007A62C | 0xF40 (mode) | 0xF9C (index) | Level[0] @ 0x000 |
| func_8007A9B0 | 0x8007A9B0 | 0xF31 (count) | Level[n]+0x0C | Enumerates level_index |
| func_8007ACDC | 0x8007ACDC | 0xF32 (count) | - | Asset count accessor |

### Observed Startup Sequence

1. `LoadBLBHeader` reads sectors 0-1 (header) to 0x800AE3E0
2. `func_8007ABCC`/`func_8007AC54` read 0xF36, 0xF92 (credits window 0)
3. `CdBLB_ReadSectors` loads bytes 0x1000-0x5000 (sector 2+)
4. After some processing, window advances:
   - 0xF37, 0xF93 (credits window 1)
5. Movie sequence starts:
   - `GetMovieFilename` reads 0xF38, 0xF94
   - `GetMovieUnknown00` reads Movie[0].reserved @ 0xB60
   - `GetMovieSectorCount` reads Movie[0].sector_count @ 0xB62
6. Each movie advances: 0xF39/0xF95 → 0xF3A/0xF96 → 0xF3B/0xF97
7. Credits window at 0xF3F/0xF9B before menu loads
8. Level parsing at 0xF40/0xF9C for menu (level 0)
9. `func_8007A9B0` enumerates all levels via 0xF31 (level count = 26)


## TODO: Research Original Compiler

The current build uses GCC 2.7.2 with maspsx, but register allocation differs from
the original binary in some functions (e.g., `GetMovieUnknown00`, `GetMovieSectorCount`).
These functions currently use `INCLUDE_ASM` to achieve byte-matching.

### Research Tasks

1. **Test other PSY-Q SDK compilers:**
   - PSYQ 3.5, 3.6, 4.0, 4.1, 4.3, 4.4, 4.5, 4.6
   - Check if any produce matching register allocation

2. **Test GCC variants:**
   - GCC 2.6.x, 2.7.x, 2.8.x, 2.91, 2.95.x
   - Try different -O levels (-O1, -O2, -O3)
   - Try with/without specific flags (-fforce-mem, -fforce-addr, etc.)

3. **Analyze register patterns:**
   - Original uses $a1 for header, $v0 for result
   - GCC 2.7.2 uses $a2 for header, $a1 for result
   - This suggests a different register allocator preference

4. **Check for compiler hints in binary:**
   - Look for compiler-specific patterns or padding
   - Compare with known PSX game compilations

5. **Tools to try:**
   - decomp.me for collaborative matching
   - Compare with other Neverhood/DreamWorks PSX decomps if available

---

## NON_MATCHING Build Analysis

Running `make verify` builds with `-DNON_MATCHING` to use the decompiled C code instead of `INCLUDE_ASM`.

### Results (2026-01-02)

| Variant | Size | SHA1 |
|---------|------|------|
| Original | 618,496 bytes | 5a14b65cb44813bfed1ee53c6a3f4456bc230f97 |
| NON_MATCHING | 618,428 bytes | dddaea850ef84f50d13117a1d4615f11ba123e56 |

**Size difference: -68 bytes** (NON_MATCHING is smaller)

### Why the Differences?

The decompiled C code is **functionally equivalent** but produces different machine code due to:

1. **Pointer/Jump Tables (0x800104D8 - 0x80012CF4)**
   - Located in `.rodata` section
   - Contain function pointers and jump table entries
   - When code is 68 bytes smaller, all function addresses shift
   - Every function pointer in the binary must change

2. **GP-relative Offsets**
   - Instructions accessing `.sdata` globals use `lw $reg, offset($gp)`
   - When the binary layout changes, GP-relative offsets change
   - Example: `lw $a0, 0x0000($gp)` vs `lw $a0, 0xFFBC($gp)`

3. **JAL Targets**
   - `jal` (jump-and-link) instructions contain absolute addresses
   - When functions shift, these addresses must change

4. **Instruction Sequence Differences**
   - Compiler may choose different register allocation
   - Different instruction scheduling/ordering
   - Missing or extra NOPs for pipeline hazards

### Affected Areas

| File Offset | RAM Address | Section | Description |
|-------------|-------------|---------|-------------|
| 0x00000CD8 | 0x800104D8 | .rodata | Jump table entries |
| 0x000010DC | 0x800108DC | .rodata | Function pointers |
| 0x00001450 | 0x80010C50 | .rodata | Jump table entries |
| 0x00003148 | 0x80012948 | .rodata | Function pointer table |
| 0x000034F4 | 0x80012CF4 | .rodata | Switch jump table |
| 0x00003970 | 0x80013170 | .rodata | Function pointer array |
| 0x00003A1C | 0x8001321C | .text | GP-relative load in func_80013200 |
| 0x00003A50 | 0x80013250 | .text | JAL target in SsUtReverbOn |
| 0x00003A98 | 0x80013298 | .text | JAL target in func_80013268 |
| 0x00003ADC | 0x800132DC | .text | JAL target in func_80013268 |

### Key Insight

The **logic of the decompiled C code is correct**. The differences are purely due to:
- Different instruction sequences (68 bytes smaller total)
- Ripple effect of address changes throughout the binary
- GP-relative addressing offsets needing adjustment

This is why `INCLUDE_ASM` is used for byte-matching builds - it preserves the exact original instruction sequences.

### m2c Decompiler Quirks

The m2c decompiler can be fooled by MIPS delay slot optimizations:

```asm
/* CdControlF takes 2 args, but m2c sees 3 */
addiu      $a0, $zero, 0x2      # $a0 = 2
addiu      $a1, $sp, 0x10       # $a1 = &pos
jal        CdControlF
 addu      $a2, $zero, $zero    # $a2 = 0 (delay slot!)
```

The compiler reuses `$a2` in the delay slot as an optimization, but `CdControlF` only uses 2 arguments. m2c incorrectly assumed this was a third argument.

**Always verify function signatures against PSY-Q headers!**
