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

# `extern <type> NAME[...] asm("D_8009xxxx")` aliases: many wrappers pass a named
# *config* array (e.g. SCALED_PLATFORM_CONFIG_MOVING) that begins with the entity's
# sprite hashes. Map the C name -> underlying D_8009 symbol so the array resolves.
ALIAS = {}   # C identifier -> "D_8009xxxx"
_ALIAS_RE = re.compile(r'\bextern\s+[\w ]+\b(\w+)\s*(?:\[\s*\])?\s*asm\(\s*"(D_8009[0-9A-Fa-f]{4})"\s*\)')
for path in glob.glob(str(REPO / 'src/*.c')):
    for line in pathlib.Path(path).read_text().splitlines():
        m = _ALIAS_RE.search(line)
        if m:
            ALIAS[m.group(1)] = m.group(2)
ALIAS_TOK = re.compile(r'\b(' + '|'.join(re.escape(k) for k in ALIAS) + r')\b') if ALIAS else None

# 2) associate every sprite-set call with its enclosing top-level C function
SPRITE_CALL = re.compile(r'\b(SetEntitySpriteId|InitEntitySprite|SetAnimationSpriteId|InitEntityWithSprite)\s*\(([^;]*)\)')
FUNC_DEF = re.compile(r'^[A-Za-z_][\w \*]*\b(\w+)\s*\([^;]*\)\s*\{?\s*$')
HASH = re.compile(r'0x[0-9A-Fa-f]{6,8}')
ARRSYM = re.compile(r'\b(D_8009[0-9A-Fa-f]{4}|g_[A-Za-z0-9_]*[Ss]prite[A-Za-z0-9_]*|[A-Za-z_]\w*Sprites)\b')

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
        # named config-array aliases passed to inner inits (whole-line; skip the
        # `extern ... asm(...)` declaration lines themselves)
        if ALIAS_TOK is not None and 'asm(' not in line:
            for tok in ALIAS_TOK.findall(line):
                fn_arrays[cur].add(ALIAS[tok])

# 2b) same, but from the disassembly — covers inner inits still INCLUDE_ASM
# (shelved / not yet decompiled), so coverage no longer waits on a byte-match.
# We read the hash straight out of the lui immediate `(0xHASH >> 16)` and any
# `%hi(D_8009xxxx)` sprite-array pointer, only in functions that actually call a
# sprite-init. Every hash is still validated against the BLB set H below.
HASH_HI = re.compile(r'\(0x([0-9A-Fa-f]{6,8}) >> 16\)')
ARR_HI = re.compile(r'%hi\((D_8009[0-9A-Fa-f]{4})\)')
SPRITE_JAL = re.compile(r'\bjal\s+(InitEntitySprite|InitEntityWithSprite|'
                        r'SetEntitySpriteId|SetAnimationSpriteId|CreateMultiFrameRenderContext)\b')
for path in glob.glob(str(REPO / 'asm/nonmatchings/**/*.s'), recursive=True):
    txt = pathlib.Path(path).read_text()
    # Scan any file that either calls a sprite-init directly (SPRITE_JAL) OR
    # loads a sprite-array pointer (%hi(D_8009xxxx)). The array may be handed to
    # a non-obvious wrapper (e.g. CreateCollectibleFromSpawn), so gating solely
    # on SPRITE_JAL misses those; every hash is still H-validated below.
    if not (SPRITE_JAL.search(txt) or ARR_HI.search(txt) or HASH_HI.search(txt)):
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
    m = re.match(r'D_(8009[0-9A-Fa-f]{4})', sym)   # full 0x8009xxxx address
    if not m:
        return []
    addr = int(m.group(1), 16)
    # A sprite-list array MUST start at a real hash — otherwise this D_8009xxxx
    # is some other data blob and we drop it (avoids stray H coincidences now
    # that the SPRITE_JAL gate is gone).
    if rom_u32(addr) not in H:
        return []
    out = []
    for i in range(16):
        v = rom_u32(addr + i * 4)
        if v in H:
            out.append(v)
        else:
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

type_hashes = collections.defaultdict(set)   # internal type -> {hash}
type_src    = {}                             # internal type -> provenance note

# (a) callback-table-rooted resolution — GROUND TRUTH.
# The entity-type callback table (entity_callbacks.json) maps each *internal*
# type to its init function's ROM address; symbol_addrs.txt resolves that address
# to the real function symbol. We root the sprite-hash search at that real
# function, so sprites land on the correct internal type even when the function
# name embeds a different (guessed) number or serves several internal types
# (e.g. EntityType039_052_FloatingEnemy_Init @0x80080c8c -> internal 39 AND 52).
# This replaces the old parse-the-number-out-of-the-name heuristic, which
# mislabelled every remapped / multi-type init.
addr2name = {}
for line in (REPO / 'symbol_addrs.txt').read_text().splitlines():
    m = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line)
    if m:
        addr2name.setdefault(int(m.group(2), 16), m.group(1))

CALLBACKS = json.loads((REPO / 'tools/entity_viewer/entity_callbacks.json').read_text())
for tk, cb in CALLBACKS.items():
    if not cb or not cb.get('init_addr'):
        continue
    t = int(tk)
    fn = addr2name.get(int(cb['init_addr'], 16))
    if not fn:
        continue
    hs = hashes_for(fn)
    if hs:
        type_hashes[t] |= hs
        type_src[t] = f"cb:{fn}"

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

# (c) join: callback root fn -> inner Init* -> documented array.
# Rooted at the callback table's real init function (same ground-truth mapping
# as (a)) so the documented-array sprites land on the correct internal type.
def reachable(fn, seen=None):
    seen = seen or set()
    if fn in seen:
        return seen
    seen.add(fn)
    for inner in fn_calls.get(fn, set()):
        reachable(inner, seen)
    return seen

for tk, cb in CALLBACKS.items():
    if not cb or not cb.get('init_addr'):
        continue
    t = int(tk)
    root = addr2name.get(int(cb['init_addr'], 16))
    if not root:
        continue
    for inner in reachable(root):
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
