#!/usr/bin/env python3
"""Build tools/entity_viewer/entity_sprite_map.json: entity_type -> sprite hashes,
extracted from the decompiled C + documented g_SpriteList arrays (read from the ROM),
every hash validated against the set that exists in the BLB. Run from repo root.

ORIGINAL DOCSTRING:
Extract entity_type -> sprite-hash mapping from the decompiled C source.

Chain: EntityTypeNNN_*_Init  ->  inner Init*()  ->  SetEntitySpriteId(0xHASH)
       or InitEntityWithSprite(e, <array>, ...) where <array> is a g_*Sprites /
       D_8009xxxx symbol whose .word contents live in the ROM data section.

Every candidate hash is validated against the set of sprite ids that actually
exist in the BLB containers, so we never record a constant that isn't a sprite.
"""
import re, json, glob, struct, pathlib, collections

REPO = pathlib.Path(__file__).resolve().parents[2]
ROM = (REPO / 'bin/SLES_010.90').read_bytes()
RAM_BASE, ROM_OFF = 0x80010000, 0x800

def rom_u32(addr):
    off = addr - RAM_BASE + ROM_OFF
    if 0 <= off <= len(ROM) - 4:
        return struct.unpack_from('<I', ROM, off)[0]
    return None

# 1) universal sprite-hash set from the BLB
H = set()
for f in glob.glob(str(REPO / 'extracted/*/*/map/600_*.bin')) + glob.glob(str(REPO / 'extracted/*/shared/600_*.bin')):
    d = pathlib.Path(f).read_bytes()
    n = struct.unpack_from('<I', d, 0)[0]
    if n > 4096 or 4 + n * 12 > len(d):
        continue
    for i in range(n):
        sid, *_ = struct.unpack_from('<III', d, 4 + i * 12)
        H.add(sid)
print(f"universal BLB sprite hashes: {len(H)}")

# SPR_* named sprite-id constants (include/Game/asset_ids.h). Many entity inits
# pass the sprite by name (e.g. InitGenericSpriteEntity(e, sp, SPR_PLATFORM_BALL_C))
# rather than a raw 0x.. literal, so the C-scan must resolve these.
SPR_DEFS = {}
_ah = REPO / 'include/Game/asset_ids.h'
if _ah.exists():
    for line in _ah.read_text().splitlines():
        m = re.match(r'#define\s+(SPR_\w+)\s+(0x[0-9A-Fa-f]+)', line)
        if m:
            SPR_DEFS[m.group(1)] = int(m.group(2), 16)
SPR_TOK = re.compile(r'\bSPR_[A-Z0-9_]+\b')

# 2) associate every sprite-set call with its enclosing top-level C function
SPRITE_CALL = re.compile(r'\b(SetEntitySpriteId|InitEntitySprite|SetAnimationSpriteId|InitEntityWithSprite)\s*\(([^;]*)\)')
FUNC_DEF = re.compile(r'^[A-Za-z_][\w \*]*\b(\w+)\s*\([^;]*\)\s*\{?\s*$')
HASH = re.compile(r'0x[0-9A-Fa-f]{6,8}')
ARRSYM = re.compile(r'\b(D_8009[0-9A-Fa-f]{3}|g_[A-Za-z0-9_]*[Ss]prite[A-Za-z0-9_]*|[A-Za-z_]\w*Sprites)\b')

fn_hashes = collections.defaultdict(set)   # function -> {hash}
fn_arrays = collections.defaultdict(set)   # function -> {array symbol}
fn_calls  = collections.defaultdict(set)   # function -> {called Init* names}
INNER = re.compile(r'\b(Init[A-Za-z0-9_]+)\s*\(')

for path in glob.glob(str(REPO / 'src/*.c')):
    cur = None
    for line in pathlib.Path(path).read_text().splitlines():
        m = FUNC_DEF.match(line)
        if m and not line.lstrip().startswith(('if', 'for', 'while', 'switch', 'return')):
            cur = m.group(1); continue
        if cur is None:
            continue
        for call in SPRITE_CALL.finditer(line):
            args = call.group(2)
            for h in HASH.findall(args):
                v = int(h, 16)
                if v in H:
                    fn_hashes[cur].add(v)
            for a in ARRSYM.findall(args):
                fn_arrays[cur].add(a)
        for ic in INNER.finditer(line):
            fn_calls[cur].add(ic.group(1))
        for tok in SPR_TOK.findall(line):       # named sprite-id constants
            v = SPR_DEFS.get(tok)
            if v is not None and v in H:
                fn_hashes[cur].add(v)

# 2b) same, but from the disassembly — covers inner inits still INCLUDE_ASM
# (shelved / not yet decompiled), so coverage no longer waits on a byte-match.
# We read the hash straight out of the lui immediate `(0xHASH >> 16)` and any
# `%hi(D_8009xxxx)` sprite-array pointer, only in functions that actually call a
# sprite-init. Every hash is still validated against the BLB set H below.
HASH_HI = re.compile(r'\(0x([0-9A-Fa-f]{6,8}) >> 16\)')
ARR_HI = re.compile(r'%hi\((D_8009[0-9A-Fa-f]{3})\)')
SPRITE_JAL = re.compile(r'\bjal\s+(InitEntitySprite|InitEntityWithSprite|'
                        r'SetEntitySpriteId|SetAnimationSpriteId|CreateMultiFrameRenderContext)\b')
for path in glob.glob(str(REPO / 'asm/nonmatchings/**/*.s'), recursive=True):
    txt = pathlib.Path(path).read_text()
    if not SPRITE_JAL.search(txt):
        continue
    fn = pathlib.Path(path).stem
    for m in HASH_HI.finditer(txt):
        v = int(m.group(1), 16)
        if v in H:
            fn_hashes[fn].add(v)
    for m in ARR_HI.finditer(txt):
        fn_arrays[fn].add(m.group(1))
    for m in re.finditer(r'\bjal\s+(Init[A-Za-z0-9_]+)', txt):
        fn_calls[fn].add(m.group(1))

# 3) resolve array symbols -> hash list from ROM
def resolve_array(sym):
    m = re.match(r'D_8009([0-9A-Fa-f]{3})', sym)
    if not m:
        return []
    addr = 0x80090000 | int(m.group(1), 16)
    out = []
    for i in range(16):
        v = rom_u32(addr + i * 4)
        if not v:
            break
        if v in H:
            out.append(v)
        elif out:
            break
    return out

# 4) wrapper EntityTypeNNN_*_Init -> inner init -> hashes (1-2 hops)
def hashes_for(fn, depth=0, seen=None):
    seen = seen or set()
    if fn in seen or depth > 3:
        return set()
    seen.add(fn)
    out = set(fn_hashes.get(fn, set()))
    for a in fn_arrays.get(fn, set()):
        out.update(resolve_array(a))
    for inner in fn_calls.get(fn, set()):
        if inner != fn and (inner in fn_hashes or inner in fn_arrays or inner in fn_calls):
            out.update(hashes_for(inner, depth + 1, seen))
    return out

TYPE_INIT = re.compile(r'EntityType(\d+)_[A-Za-z0-9_]*Init')
type_hashes = collections.defaultdict(set)   # type -> {hash}
type_src    = {}                             # type -> provenance note

# (a) direct hashes resolved from the C wrapper/inner chain
for fn in list(fn_hashes) + list(fn_arrays) + list(fn_calls):
    m = TYPE_INIT.match(fn)
    if not m:
        continue
    t = int(m.group(1))
    hs = hashes_for(fn)
    if hs:
        type_hashes[t] |= hs
        type_src[t] = f"C:{fn}"

# (b) documented g_SpriteList_* arrays read straight from the ROM
DOC = (REPO / 'docs/systems/sprites.md').read_text().splitlines()
ROW = re.compile(r'\|\s*(0x8009[0-9A-Fa-f]+)\s*\|\s*(\d+)\s*\|\s*\**([A-Za-z0-9_]+)\**\s*\|\s*([^|]*)\|')
init_to_arr = {}   # inner init fn name -> (addr, count, label)
for line in DOC:
    m = ROW.match(line)
    if not m:
        continue
    addr, cnt, label, initcol = int(m.group(1), 16), int(m.group(2)), m.group(3), m.group(4)
    hs = [v for i in range(cnt) if (v := rom_u32(addr + i * 4)) in H]
    if not hs:
        continue
    # b1: explicit TypeNNN in the array label
    for t in re.findall(r'Type(\d+)', label):
        type_hashes[int(t)] |= set(hs)
        type_src.setdefault(int(t), f"arr:{label}")
    # b2: remember the inner-init fn this array feeds, for the call-graph join
    fnm = re.match(r'\s*([A-Za-z_]\w+)', initcol)
    if fnm:
        init_to_arr[fnm.group(1)] = set(hs)

# (c) join: type wrapper -> inner Init* -> documented array
for fn in list(fn_calls):
    m = TYPE_INIT.match(fn)
    if not m:
        continue
    t = int(m.group(1))
    for inner in fn_calls.get(fn, set()):
        if inner in init_to_arr:
            type_hashes[t] |= init_to_arr[inner]
            type_src.setdefault(t, f"arr-via:{inner}")

type_map = {t: {"hashes": [f"0x{h:08x}" for h in sorted(hs)], "via": type_src.get(t, "?")}
            for t, hs in type_hashes.items() if hs}

print(f"types with >=1 resolved sprite hash: {len(type_map)}")
for t in sorted(type_map):
    print(f"  type {t:>3}: {str(type_map[t]['hashes']):<60} [{type_map[t]['via']}]")
out = REPO / 'tools/entity_viewer/entity_sprite_map.json'
out.write_text(json.dumps({str(k): v for k, v in sorted(type_map.items())}, indent=1))
print("wrote", out)
