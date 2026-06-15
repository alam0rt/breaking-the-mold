#!/usr/bin/env python3
"""Probe Skullmonkeys 32-bit asset IDs.

This is an analysis helper for the asset-hash investigation. It parses Skullmonkeys
BLB files directly, collects sprite/audio/animation IDs, and runs a small
role-aware dictionary test over sparse bit-toggle hash variants.
"""
from __future__ import annotations

import argparse
import collections
import csv
import itertools
import hashlib
import re
import statistics
import struct
from pathlib import Path

SECTOR = 2048
ROOT = Path(__file__).resolve().parents[2]
SEEDED_OUTPUT_ROT = 27
SEEDED_XOR_MASK = 0x28C0E011

ANCHORS = {
    "player": (0x21842018, ["KLAYMEN", "KLAYMAN", "KLAY", "PLAYER", "HERO", "CHARACTER", "MAIN", "KLAYMENPLAYER", "SKULLMONKEY"]),
    "green_bullets": (0xB8700CA1, ["GREENBULLET", "GREENBULLETS", "BULLET", "BULLETS", "AMMO", "GREENAMMO", "ENERGYBALL", "ENERGYBALLS", "PROJECTILE", "WEAPON"]),
    "phoenix_hand": (0x9158A0F6, ["PHOENIXHAND", "PHOENIX", "HAND", "BIRD", "THEBIRD", "SEEKER", "HOMINGBIRD"]),
    "phart_head": (0x8C510186, ["PHARTHEAD", "FARTHEAD", "PHART", "FART", "GASHEAD", "CLONE", "FARTCLONE", "PHARTCLONE"]),
    "one_up": (0xA9240484, ["ONEUP", "1UP", "EXTRALIFE", "LIFE", "HEAD", "KLAYMENHEAD", "KLAYHEAD", "BONUSLIFE"]),
    "klogg_ball": (0x0C34AA22, ["KLOGGBALL", "KLOGG", "BALL", "CATCHBALL", "CATCHABLEBALL", "SPIKEBALL", "SPIKYBALL"]),
    "moving_platform_a": (0x88783718, ["MOVINGPLATFORM", "MOVINGPLATFORMA", "PLATFORM", "PLATFORMA", "MOVER", "HOVERPLATFORM", "LIFT", "MOVPLAT"]),
    "sparkle": (0x6A351094, ["SPARKLE", "SPARKLES", "TWINKLE", "COLLECT", "COLLECTFX", "PICKUPFX", "FXSPARKLE"]),
    "jump_sound": (0x00248E52, ["JUMP", "JUMPING", "BOING", "HOP", "LEAP", "KLAYJUMP", "PLAYERJUMP"]),
    "pickup_sound": (0x7003474C, ["PICKUP", "PICKUPITEM", "COLLECT", "COLLECTITEM", "GET", "ITEM", "GRAB", "KLAYPICKUP"]),
    "projectile": (0x168254B5, ["PROJECTILE", "BULLET", "GREENBULLET", "SHOT", "AMMO", "FIREBALL", "ENERGYBALL"]),
}

PREFIXES = ["", "S", "SPR", "SEQ", "SND", "SFX", "FX", "OBJ", "ENT", "ITEM", "N2", "SM", "SK", "KLAY", "KL", "PHRO", "MENU"]
SUFFIXES = ["", "0", "1", "01", "A", "B", "C", "SPR", "SEQ", "SND", "SFX", "ANIM", "ID", "OBJ", "ITEM", "FX"]
EXTS = ["", ".SPR", ".SEQ", ".SND", ".VAG", ".TIM", ".ANM", ".CEL", ".DAT", ".BIN", ".RAW", ".XA"]
SEPS = ["", "_", "-"]


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def valid_toc(data: bytes, seg_off: int, seg_len: int):
    if seg_off < 0 or seg_off + 4 > len(data):
        return None
    count = u32(data, seg_off)
    if not (0 < count < 64) or seg_off + 4 + count * 12 > len(data):
        return None
    entries = []
    for i in range(count):
        typ, size, rel = struct.unpack_from("<III", data, seg_off + 4 + i * 12)
        if typ > 2000 or rel < 4 + count * 12 or rel + size > seg_len or seg_off + rel + size > len(data):
            return None
        entries.append((typ, size, rel))
    return entries


def parse_container(data: bytes, abs_off: int, size: int, kind: str, rows):
    if size < 4 or abs_off + 4 > len(data):
        return
    count = u32(data, abs_off)
    if not (0 <= count < 2000) or 4 + count * 12 > size:
        return
    for i in range(count):
        _entry_id, entry_size, entry_rel = struct.unpack_from("<III", data, abs_off + 4 + i * 12)
        if entry_rel + entry_size > size:
            return
    for i in range(count):
        entry_id, entry_size, entry_rel = struct.unpack_from("<III", data, abs_off + 4 + i * 12)
        rows[kind].append(entry_id)
        if kind == "sprite" and entry_rel + 12 <= size:
            anim_count = u16(data, abs_off + entry_rel)
            if 0 < anim_count < 256 and entry_rel + 12 + anim_count * 12 <= size:
                for j in range(anim_count):
                    anim_id = u32(data, abs_off + entry_rel + 12 + j * 12)
                    rows["anim"].append(anim_id)


def collect_blb(path: Path):
    data = path.read_bytes()
    rows = collections.defaultdict(list)
    segments = []
    for level in range(26):
        base = level * 0x70
        primary_sector, primary_count = u16(data, base), u16(data, base + 2)
        stage_count = u16(data, base + 0x0E)
        if 0 < primary_count < 10000:
            segments.append(("primary", level, -1, primary_sector * SECTOR, primary_count * SECTOR))
        if not (0 < stage_count <= 6):
            continue
        for stage in range(stage_count):
            sec_sector, sec_count = u16(data, base + 0x1E + stage * 2), u16(data, base + 0x2C + stage * 2)
            tert_sector, tert_count = u16(data, base + 0x3A + stage * 2), u16(data, base + 0x48 + stage * 2)
            if sec_count:
                segments.append(("secondary", level, stage, sec_sector * SECTOR, sec_count * SECTOR))
            if tert_count:
                segments.append(("tertiary", level, stage, tert_sector * SECTOR, tert_count * SECTOR))
    for seg_kind, level, stage, seg_off, seg_len in segments:
        toc = valid_toc(data, seg_off, seg_len)
        if toc is None:
            continue
        for typ, size, rel in toc:
            if typ == 600:
                parse_container(data, seg_off + rel, size, "sprite", rows)
            elif typ == 601:
                parse_container(data, seg_off + rel, size, "audio", rows)
    return rows


def level_code(data: bytes, level: int) -> str:
    raw = data[level * 0x70 + 0x56:level * 0x70 + 0x5B]
    return raw.split(b"\0", 1)[0].decode("ascii", "replace")


def collect_located_entries(path: Path):
    data = path.read_bytes()
    rows = {}
    segments = []
    for level in range(26):
        base = level * 0x70
        primary_sector, primary_count = u16(data, base), u16(data, base + 2)
        stage_count = u16(data, base + 0x0E)
        code = level_code(data, level)
        if 0 < primary_count < 10000:
            segments.append((code, "primary", -1, primary_sector * SECTOR, primary_count * SECTOR))
        if not (0 < stage_count <= 6):
            continue
        for stage in range(stage_count):
            sec_sector, sec_count = u16(data, base + 0x1E + stage * 2), u16(data, base + 0x2C + stage * 2)
            tert_sector, tert_count = u16(data, base + 0x3A + stage * 2), u16(data, base + 0x48 + stage * 2)
            if sec_count:
                segments.append((code, "secondary", stage, sec_sector * SECTOR, sec_count * SECTOR))
            if tert_count:
                segments.append((code, "tertiary", stage, tert_sector * SECTOR, tert_count * SECTOR))
    for code, seg_kind, stage, seg_off, seg_len in segments:
        toc = valid_toc(data, seg_off, seg_len)
        if toc is None:
            continue
        for asset_index, (typ, size, rel) in enumerate(toc):
            if typ not in (600, 601):
                continue
            abs_off = seg_off + rel
            if size < 4 or abs_off + 4 > len(data):
                continue
            count = u32(data, abs_off)
            if not (0 <= count < 2000) or 4 + count * 12 > size:
                continue
            for entry_index in range(count):
                entry_id, entry_size, entry_rel = struct.unpack_from("<III", data, abs_off + 4 + entry_index * 12)
                if entry_rel + entry_size > size:
                    break
                payload = data[abs_off + entry_rel:abs_off + entry_rel + entry_size]
                key = (code, seg_kind, stage, typ, asset_index, entry_index)
                rows[key] = {
                    "id": entry_id,
                    "size": entry_size,
                    "sha1": hashlib.sha1(payload).hexdigest(),
                }
    return rows


def bitrev(value: int) -> int:
    out = 0
    for i in range(32):
        out = (out << 1) | ((value >> i) & 1)
    return out


def bswap(value: int) -> int:
    return int.from_bytes(value.to_bytes(4, "little"), "big")


def rot(value: int, amount: int) -> int:
    amount &= 31
    return ((value << amount) | (value >> ((32 - amount) & 31))) & 0xFFFFFFFF


def char_value(ch: str, mode: str):
    o = ord(ch)
    if mode == "never":
        if "a" <= ch <= "z":
            o -= 32
        elif "0" <= ch <= "9":
            o += 22
        if ("A" <= ch <= "Z") or ("a" <= ch <= "z") or ("0" <= ch <= "9"):
            return (o - 64) & 31
        return None
    if mode == "toolx":
        if "a" <= ch <= "z":
            o -= 32
        elif "0" <= ch <= "9":
            o += 22
        return (o - 64) & 31
    if mode == "alnumraw":
        return o & 31 if ch.isalnum() else None
    if mode == "printupper":
        if "a" <= ch <= "z":
            o -= 32
        return o & 31 if 32 <= o < 127 else None
    if mode == "allraw":
        return o & 31
    raise ValueError(mode)


def cumulative_toggle(text: str, mode: str, pre_toggle: bool = False) -> int:
    h = 0
    shift = 0
    for ch in text:
        v = char_value(ch, mode)
        if v is None:
            continue
        if pre_toggle:
            h ^= 1 << shift
        shift = (shift + v) & 31
        if not pre_toggle:
            h ^= 1 << shift
    return h


def set_toggle(text: str, mode: str) -> int:
    h = 0
    for ch in text:
        v = char_value(ch, mode)
        if v is not None:
            h ^= 1 << v
    return h


def seeded_name_hash(text: str) -> int:
    """Current best Skullmonkeys hash candidate from the YES/NO visual anchors."""
    return SEEDED_XOR_MASK ^ rot(cumulative_toggle(text, "never", False), SEEDED_OUTPUT_ROT)


def candidate_names(tokens):
    out = set()
    for token in tokens:
        base = re.sub(r"[^A-Za-z0-9]", "", token).upper()
        variants = {base, base.lower(), base.capitalize()}
        for variant in variants:
            for ext in EXTS:
                out.add(variant + ext)
            for prefix, sep in itertools.product(PREFIXES, SEPS):
                if prefix:
                    stem = prefix + sep + variant
                    out.add(stem)
                    for ext in EXTS:
                        out.add(stem + ext)
            for suffix, sep in itertools.product(SUFFIXES, SEPS):
                if suffix:
                    stem = variant + sep + suffix
                    out.add(stem)
                    for ext in EXTS:
                        out.add(stem + ext)
    return out


def hash_variants():
    for mode in ["never", "toolx", "alnumraw", "printupper", "allraw"]:
        yield f"cum-{mode}", lambda s, m=mode: cumulative_toggle(s, m, False)
        yield f"pre-{mode}", lambda s, m=mode: cumulative_toggle(s, m, True)
        yield f"set-{mode}", lambda s, m=mode: set_toggle(s, m)


def post_variants():
    posts = [
        ("id", lambda x: x),
        ("bitrev", bitrev),
        ("bswap", bswap),
        ("bswap_bitrev", lambda x: bswap(bitrev(x))),
    ]
    for post_name, post_fn in posts:
        for amount in range(32):
            name = post_name if amount == 0 else f"{post_name}_rot{amount}"
            yield name, lambda x, p=post_fn, r=amount: rot(p(x), r)


def target_preimages(target: int) -> dict[int, str]:
    """Return all base hashes that can post-transform to target."""
    out = {}
    for amount in range(32):
        unrotated = rot(target, (-amount) & 31)
        suffix = "" if amount == 0 else f"_rot{amount}"
        out[unrotated] = "id" + suffix
        out[bitrev(unrotated)] = "bitrev" + suffix
        out[bswap(unrotated)] = "bswap" + suffix
        out[bitrev(bswap(unrotated))] = "bswap_bitrev" + suffix
    return out


def print_pool_summary(blb_paths):
    for path in blb_paths:
        rows = collect_blb(path)
        all_ids = set(rows["sprite"]) | set(rows["anim"]) | set(rows["audio"])
        print(f"\n{path.relative_to(ROOT)} size={path.stat().st_size}")
        for kind in ["sprite", "anim", "audio"]:
            unique = set(rows[kind])
            if unique:
                popcounts = [value.bit_count() for value in unique]
                print(f"  {kind:6s} entries={len(rows[kind]):5d} unique={len(unique):4d} avg_pop={statistics.mean(popcounts):5.2f} min={min(popcounts):2d} max={max(popcounts):2d}")
            else:
                print(f"  {kind:6s} entries=0 unique=0")
        if all_ids:
            print(f"  all    unique={len(all_ids):4d} avg_pop={statistics.mean([value.bit_count() for value in all_ids]):5.2f}")


def print_dictionary_probe():
    variants = list(hash_variants())
    for label, (target, tokens) in ANCHORS.items():
        hits = []
        preimages = target_preimages(target)
        names = candidate_names(tokens)
        for name in names:
            for variant_name, hash_fn in variants:
                base_hash = hash_fn(name)
                post_name = preimages.get(base_hash)
                if post_name is not None:
                    hits.append((name, variant_name, post_name, base_hash))
                    if len(hits) >= 20:
                        break
            if len(hits) >= 20:
                break
        print(f"\n{label:18s} target=0x{target:08x} candidates={len(names)} hits={len(hits)}")
        for name, variant_name, post_name, base_hash in hits[:10]:
            print(f"  {name:24s} {variant_name:16s} {post_name:20s} base=0x{base_hash:08x}")


def print_seeded_probe() -> None:
    """Probe the seeded/rotated calcHash candidate derived from YES/NO."""
    no_id = 0x29C0E211
    yes_id = 0x2AD0F011
    no_base = cumulative_toggle("NO", "never", False)
    yes_base = cumulative_toggle("YES", "never", False)
    expected_delta = rot(no_base ^ yes_base, SEEDED_OUTPUT_ROT)
    observed_delta = no_id ^ yes_id
    no_seed = no_id ^ rot(no_base, SEEDED_OUTPUT_ROT)
    yes_seed = yes_id ^ rot(yes_base, SEEDED_OUTPUT_ROT)

    print("YES/NO seeded-hash probe")
    print(f"  NO  id=0x{no_id:08x} base=0x{no_base:08x}")
    print(f"  YES id=0x{yes_id:08x} base=0x{yes_base:08x}")
    print(f"  observed NO^YES:  0x{observed_delta:08x}")
    print(f"  expected NO^YES:  0x{expected_delta:08x} = rotl(calcHash('NO')^calcHash('YES'), {SEEDED_OUTPUT_ROT})")
    print(f"  implied seed from NO:  0x{no_seed:08x}")
    print(f"  implied seed from YES: 0x{yes_seed:08x}")
    print(f"  candidate formula: id = 0x{SEEDED_XOR_MASK:08x} ^ rotl(calcHash(name), {SEEDED_OUTPUT_ROT})")

    rows = collect_blb(ROOT / "disks/blb/sles-01090.blb")
    all_ids = set(rows["sprite"]) | set(rows["anim"]) | set(rows["audio"])
    labels = {}
    csv_path = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if csv_path.exists():
        with csv_path.open(newline="") as f:
            for row in csv.DictReader(f):
                try:
                    sprite_id = int(row["sprite_hex"], 16)
                except Exception:
                    continue
                if row.get("human_role"):
                    labels[sprite_id] = row["human_role"]

    checks = [
        "NO", "YES",
        "CURSOR", "MENU_CURSOR", "MENUCURSOR", "POINTER", "SELECTOR",
        "MONKEY", "SCREAMER", "SCREAMERMONKEY", "SCREAMER_MONKEY",
        "SHRINEY", "SHRINEYGUARD", "SHRINEY_GUARD", "SHRINEYGUARD_SLAM", "SHRINEYGUARD_YELL",
        "MENU_DOOR_OPEN", "DOOROPEN", "MONKEYDOOR",
    ]
    print("\nExact-name checks:")
    for name in checks:
        value = seeded_name_hash(name)
        hit = "HIT" if value in all_ids else "-"
        label = labels.get(value, "")
        print(f"  {name:22s} -> 0x{value:08x} {hit:3s} {label}")

    candidate_tokens = {
        "NO": ["NO"],
        "YES": ["YES"],
        "cursor": ["CURSOR", "MENU_CURSOR", "MENUCURSOR", "POINTER", "SELECTOR"],
        "plain_monkey": ["MONKEY", "SCREAMER", "SCREAMERMONKEY", "SCREAMER_MONKEY", "SKULLMONKEY", "ENEMYMONKEY"],
        "shriney_guard": ["SHRINEY", "SHRINEYGUARD", "SHRINEY_GUARD", "SHINYGUARD", "SHRINEGUARD", "GUARD"],
        "menu_door": ["MENUDOOR", "MENU_DOOR", "MONKEYDOOR", "DOOR", "DOOROPEN", "MENU_DOOR_OPEN"],
    }
    print("\nCandidate grammar hits under seeded formula:")
    for label, tokens in candidate_tokens.items():
        names = candidate_names(tokens)
        names.update(tokens)
        hits = []
        for name in sorted(names, key=lambda item: (len(item), item)):
            value = seeded_name_hash(name)
            if value in all_ids:
                hits.append((name, value, labels.get(value, "")))
        print(f"  {label:14s} candidates={len(names):5d} hits={len(hits):3d}")
        for name, value, human_label in hits[:12]:
            print(f"    0x{value:08x} {name:24s} {human_label}")


def normalized_rotation(value: int) -> int:
    return min(rot(value, amount) for amount in range(32))


def print_language_probe() -> None:
    # Normalized old-English -> French/German delta masks discovered from SLES-01091/01092.
    english_to_french = 0x01079563
    english_to_german = 0x00340B0F
    english = ["E", "EN", "ENG", "UK", "US", "EU", "PAL", "EURO", "ENGLISH", "BRITISH", "AMERICAN", "USA", "SLES01090", "SLES90", "01090", "90"]
    french = ["F", "FR", "FRE", "FRA", "FRANCE", "FRENCH", "FRANCAIS", "SLES01091", "SLES91", "01091", "91"]
    german = ["G", "GE", "GER", "DE", "DEU", "GERMAN", "GERMANY", "DEUTSCH", "DEUTSCHLAND", "SLES01092", "SLES92", "01092", "92"]

    def forms(words):
        out = set()
        for word in words:
            for pre in ["", "_", "-", ".", "LANG", "LANG_", "LANG-", "L", "L_", "L-", "TEXT", "TEXT_", "VO", "VO_", "VOC", "VOC_", "PAL", "PAL_", "EU", "EU_"]:
                for suf in ["", "_", "-", ".", "LANG", "_LANG", "-LANG", "TEXT", "_TEXT", "-TEXT", "VO", "_VO", "-VO"]:
                    out.add(pre + word + suf)
        return sorted(out)

    e_forms = forms(english)
    f_forms = forms(french)
    g_forms = forms(german)
    print(f"language forms E={len(e_forms)} F={len(f_forms)} G={len(g_forms)}")
    french_delta_rotations = {rot(english_to_french, amount) for amount in range(32)}
    german_delta_rotations = {rot(english_to_german, amount) for amount in range(32)}
    for variant_name, hash_fn in hash_variants():
        e_hashes = [(text, hash_fn(text)) for text in e_forms]
        f_by_hash = collections.defaultdict(list)
        g_by_hash = collections.defaultdict(list)
        for text in f_forms:
            f_by_hash[hash_fn(text)].append(text)
        for text in g_forms:
            g_by_hash[hash_fn(text)].append(text)
        found = 0
        for e_text, e_hash in e_hashes:
            f_hits = []
            for delta in french_delta_rotations:
                f_hits.extend(f_by_hash.get(e_hash ^ delta, []))
            if not f_hits:
                continue
            g_hits = []
            for delta in german_delta_rotations:
                g_hits.extend(g_by_hash.get(e_hash ^ delta, []))
            if g_hits:
                for f_text in f_hits[:5]:
                    for g_text in g_hits[:5]:
                        print(f"HIT {variant_name}: E={e_text} F={f_text} G={g_text}")
                        found += 1
                        if found >= 20:
                            break
                    if found >= 20:
                        break
            if found >= 20:
                break
        if found:
            return
    print("no language-token hits in tested bit-toggle variants")


def print_diff(base_path: Path, other_path: Path) -> None:
    base = collect_located_entries(base_path)
    other = collect_located_entries(other_path)
    changed = []
    for key in sorted(base.keys() & other.keys()):
        a = base[key]
        b = other[key]
        if a["id"] != b["id"] or a["sha1"] != b["sha1"] or a["size"] != b["size"]:
            changed.append((key, a, b))
    print(f"diff {base_path.relative_to(ROOT)} -> {other_path.relative_to(ROOT)} changed_entries={len(changed)}")
    for key, a, b in changed[:200]:
        code, seg_kind, stage, typ, asset_index, entry_index = key
        stage_text = "-" if stage < 0 else str(stage)
        same_bytes = a["sha1"] == b["sha1"]
        print(
            f"{code:4s} {seg_kind:9s} stage={stage_text:>1s} asset={typ} "
            f"toc={asset_index:02d} entry={entry_index:03d} "
            f"0x{a['id']:08x}->{b['id']:08x} "
            f"size={a['size']}->{b['size']} same_bytes={same_bytes}"
        )


def unique_changed_id_pairs(base_path: Path, other_path: Path):
    base = collect_located_entries(base_path)
    other = collect_located_entries(other_path)
    pairs = collections.defaultdict(list)
    for key in sorted(base.keys() & other.keys()):
        old_id = base[key]["id"]
        new_id = other[key]["id"]
        if old_id != new_id:
            pairs[(old_id, new_id)].append(key)
    return pairs


def rotation_for(mask: int, value: int):
    for amount in range(32):
        if rot(mask, amount) == value:
            return amount
    return None


def print_regional_delta_probe() -> None:
    english = ROOT / "disks/blb/sles-01090.blb"
    french = ROOT / "disks/blb/sles-01091.blb"
    german = ROOT / "disks/blb/sles-01092.blb"
    fr_pairs = unique_changed_id_pairs(english, french)
    de_pairs = unique_changed_id_pairs(english, german)
    print(f"English->French unique changed sprite ids: {len(fr_pairs)}")
    print(f"English->German unique changed sprite ids: {len(de_pairs)}")

    for label, pairs in [("FR", fr_pairs), ("DE", de_pairs)]:
        classes = collections.defaultdict(list)
        for old_id, new_id in pairs:
            delta = old_id ^ new_id
            classes[normalized_rotation(delta)].append((old_id, new_id, delta))
        print(f"\n{label} rotation classes: {len(classes)}")
        for norm, values in sorted(classes.items(), key=lambda item: (-len(item[1]), item[0])):
            print(f"  norm=0x{norm:08x} count={len(values)} pop={norm.bit_count()}")
            for old_id, new_id, delta in values[:6]:
                r = rotation_for(norm, delta)
                print(f"    0x{old_id:08x}->0x{new_id:08x} delta=0x{delta:08x} rot={r}")

    fr_norm = next(iter({normalized_rotation(old_id ^ new_id) for old_id, new_id in fr_pairs}))
    de_norm = next(iter({normalized_rotation(old_id ^ new_id) for old_id, new_id in de_pairs}))
    rotation_diffs = []
    aligned_old_ids = sorted({old_id for old_id, _new_id in fr_pairs} & {old_id for old_id, _new_id in de_pairs})
    for old_id in aligned_old_ids:
        fr_new = next(new_id for pair_old, new_id in fr_pairs if pair_old == old_id)
        de_new = next(new_id for pair_old, new_id in de_pairs if pair_old == old_id)
        fr_rot = rotation_for(fr_norm, old_id ^ fr_new)
        de_rot = rotation_for(de_norm, old_id ^ de_new)
        rotation_diffs.append((de_rot - fr_rot) & 31)
    if rotation_diffs and len(set(rotation_diffs)) == 1:
        print(f"\nAligned suffix masks:")
        print(f"  FR suffix delta: 0x{fr_norm:08x}")
        print(f"  DE suffix delta: 0x{rot(de_norm, rotation_diffs[0]):08x} (DE norm rot {rotation_diffs[0]})")
    print("\nShared English id rotation alignment:")
    for old_id in aligned_old_ids:
        fr_new = next(new_id for pair_old, new_id in fr_pairs if pair_old == old_id)
        de_new = next(new_id for pair_old, new_id in de_pairs if pair_old == old_id)
        fr_rot = rotation_for(fr_norm, old_id ^ fr_new)
        de_rot = rotation_for(de_norm, old_id ^ de_new)
        print(f"  0x{old_id:08x}: FR rot={fr_rot:2d}, DE rot={de_rot:2d}, diff={(de_rot - fr_rot) & 31:2d}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pool", action="store_true", help="summarize IDs in all Skullmonkeys BLBs")
    parser.add_argument("--dict", action="store_true", help="run role-aware dictionary probe")
    parser.add_argument("--lang", action="store_true", help="probe likely English/French/German language tag deltas")
    parser.add_argument("--regional", action="store_true", help="summarize PAL English/French/German rotated delta masks")
    parser.add_argument("--seeded", action="store_true", help="probe the seeded/rotated calcHash candidate from YES/NO anchors")
    parser.add_argument("--diff", nargs=2, metavar=("BASE", "OTHER"), help="diff located container entries between two BLBs")
    args = parser.parse_args()
    if not args.pool and not args.dict and not args.lang and not args.regional and not args.seeded and not args.diff:
        args.pool = args.dict = True

    blb_paths = []
    for path in sorted((ROOT / "disks/blb").glob("*.blb")):
        blb_paths.append(path)
    for path in [ROOT / "disks/blb/GAME.BLB", ROOT / "disks/demo/N2/GAME.BLB", ROOT / "disks/prototype/ext/GAME.BLB"]:
        if path.exists() and path not in blb_paths:
            blb_paths.append(path)

    if args.pool:
        print_pool_summary(blb_paths)
    if args.dict:
        print_dictionary_probe()
    if args.lang:
        print_language_probe()
    if args.regional:
        print_regional_delta_probe()
    if args.seeded:
        print_seeded_probe()
    if args.diff:
        print_diff((ROOT / args.diff[0]).resolve(), (ROOT / args.diff[1]).resolve())


if __name__ == "__main__":
    main()
