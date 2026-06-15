#!/usr/bin/env python3
"""Build labeled sprite contact sheets for asset-hash identification.

This uses the already-extracted sprite PNG/GIFs under extracted/ and can decode
missing thumbnails directly from the English BLB. It produces:

- a focused contact sheet chunk around the PAL regional replacement IDs
- a regional-only sheet for the 23 English sprite ids replaced in FR/DE PAL BLBs
- a CSV template that can be filled with human role/name identifications

The output is intended as a visual annotation workflow: identify a sprite by its
hex id on the sheet, then fill that row in the CSV.
"""
from __future__ import annotations

import argparse
import csv
import math
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
SPRITE_RE = re.compile(r"sprite_(\d+)_anim(\d+)(?:_f(\d+))?\.(png|gif)$")

# Small fallback subset of the controlled PAL localization replacement set.
# The full 23-pair set is loaded dynamically from asset_hash_probe.py when the
# PAL BLBs are available.
FALLBACK_REGIONAL_REPLACEMENTS = {
    0x524EC094: (0x13AB9854, 0x33AEC615),
    0x542A8850: (0x4A7F0454, 0x4A2AE046),
    0x512C8430: (0x5E064232, 0x5E2CB03B),
    0x620EC210: (0x90A2A230, 0x920D82A0),
    0x4AEE0118: (0xC924B098, 0x892E0C1A),
    0x492C8430: (0x46064232, 0x462CB03B),
}


def load_regional_replacements() -> dict[int, tuple[int, int]]:
    try:
        import asset_hash_probe

        english = ROOT / "disks/blb/sles-01090.blb"
        french = ROOT / "disks/blb/sles-01091.blb"
        german = ROOT / "disks/blb/sles-01092.blb"
        if not (english.exists() and french.exists() and german.exists()):
            return dict(FALLBACK_REGIONAL_REPLACEMENTS)
        fr_pairs = dict(asset_hash_probe.unique_changed_id_pairs(english, french).keys())
        de_pairs = dict(asset_hash_probe.unique_changed_id_pairs(english, german).keys())
        return {old_id: (fr_pairs[old_id], de_pairs[old_id]) for old_id in sorted(fr_pairs.keys() & de_pairs.keys())}
    except Exception:
        return dict(FALLBACK_REGIONAL_REPLACEMENTS)


@dataclass
class SpriteImage:
    path: Path
    rel: str
    level: str
    segment: str
    anim: int
    frame: int | None
    size: tuple[int, int]
    bbox_area: int


@dataclass
class SpriteInfo:
    sprite_id: int
    images: list[SpriteImage] = field(default_factory=list)
    levels: set[str] = field(default_factory=set)
    segments: set[str] = field(default_factory=set)

    @property
    def hex(self) -> str:
        return f"0x{self.sprite_id:08x}"

    def best_image(self) -> SpriteImage | None:
        if not self.images:
            return None
        # Prefer a representative visible frame. Huge alpha-bbox images tend to be
        # the clearest for visual identification. Prefer primary segment on ties.
        def score(img: SpriteImage):
            primary_bonus = 1 if img.segment == "primary" else 0
            frame_penalty = 0 if img.frame is not None else -1
            return (img.bbox_area, primary_bonus, frame_penalty, -(img.anim), img.rel)

        return max(self.images, key=score)


def alpha_bbox_area(image: Image.Image) -> int:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return image.width * image.height
    return (bbox[2] - bbox[0]) * (bbox[3] - bbox[1])


def level_and_segment(path: Path, extracted_dir: Path) -> tuple[str, str]:
    rel_parts = path.relative_to(extracted_dir).parts
    level = ""
    segment = ""
    for i, part in enumerate(rel_parts):
        if len(part) == 4 and part.isupper():
            level = part
            if i + 1 < len(rel_parts):
                segment = rel_parts[i + 1]
            break
    return level, segment


def scan_sprites(extracted_dir: Path) -> dict[int, SpriteInfo]:
    sprites: dict[int, SpriteInfo] = {}
    for path in extracted_dir.rglob("sprite_*.*"):
        match = SPRITE_RE.match(path.name)
        if not match or match.group(4).lower() not in {"png", "gif"}:
            continue
        sprite_id = int(match.group(1))
        anim = int(match.group(2))
        frame = int(match.group(3)) if match.group(3) is not None else None
        try:
            with Image.open(path) as image:
                size = image.size
                bbox_area = alpha_bbox_area(image)
        except Exception:
            continue

        level, segment = level_and_segment(path, extracted_dir)
        rel = str(path.relative_to(extracted_dir))
        info = sprites.setdefault(sprite_id, SpriteInfo(sprite_id))
        info.images.append(SpriteImage(path, rel, level, segment, anim, frame, size, bbox_area))
        if level:
            info.levels.add(level)
        if segment:
            info.segments.add(segment)
    return sprites


def decode_sprite_payload(sprite_data: bytes) -> Image.Image | None:
    try:
        from tools.extract_blb.handlers.sprites import (
            decode_rle_sprite,
            parse_animation_entry,
            parse_frame_metadata,
            parse_palette,
            parse_sprite_header,
        )
    except Exception:
        return None

    if len(sprite_data) < 12:
        return None
    try:
        header = parse_sprite_header(sprite_data, 0)
    except Exception:
        return None
    if not (0 < header["animation_count"] < 512):
        return None
    if header["palette_offset"] + 512 > len(sprite_data):
        return None

    try:
        palette = parse_palette(sprite_data, header["palette_offset"])
    except Exception:
        return None

    best: tuple[int, Image.Image] | None = None
    frame_meta_base = header["frame_meta_offset"]
    for anim_idx in range(min(header["animation_count"], 64)):
        anim_off = 12 + anim_idx * 12
        if anim_off + 12 > len(sprite_data):
            break
        try:
            anim = parse_animation_entry(sprite_data, anim_off)
        except Exception:
            continue
        for frame_idx in range(min(anim["frame_count"], 256)):
            abs_frame_idx = anim["frame_offset"] + frame_idx
            frame_off = frame_meta_base + abs_frame_idx * 36
            if frame_off + 36 > len(sprite_data):
                continue
            try:
                frame = parse_frame_metadata(sprite_data, frame_off)
            except Exception:
                continue
            width = frame["render_width"]
            height = frame["render_height"]
            if width <= 0 or height <= 0 or width > 512 or height > 512:
                continue
            rle_abs = header["rle_offset"] + frame["rle_offset"]
            if rle_abs + 4 > len(sprite_data):
                continue
            try:
                pixels = decode_rle_sprite(sprite_data, rle_abs, width, height, palette)
                image = Image.frombytes("RGBA", (width, height), pixels)
            except Exception:
                continue
            area = alpha_bbox_area(image)
            score = area * 1_000_000 + width * height
            if best is None or score > best[0]:
                best = (score, image)
    return best[1] if best else None


def add_missing_blb_thumbnails(sprites: dict[int, SpriteInfo], out_dir: Path) -> int:
    """Decode thumbnails for BLB sprite ids that the extraction tree lacks.

    Older extraction runs sometimes produce no PNG/GIF for a valid sprite id. This
    fallback decodes the first usable frame directly from SLES-01090 so the visual
    identification sheet covers more of the actual BLB id pool.
    """
    try:
        import asset_hash_probe
    except Exception:
        return 0

    blb_path = ROOT / "disks/blb/sles-01090.blb"
    if not blb_path.exists():
        return 0
    data = blb_path.read_bytes()
    entries: dict[int, list[tuple[str, str, bytes]]] = {}
    for level in range(26):
        base = level * 0x70
        code = asset_hash_probe.level_code(data, level)
        primary_sector, primary_count = asset_hash_probe.u16(data, base), asset_hash_probe.u16(data, base + 2)
        stage_count = asset_hash_probe.u16(data, base + 0x0E)
        segments: list[tuple[str, int, int]] = []
        if 0 < primary_count < 10000:
            segments.append(("primary", primary_sector * asset_hash_probe.SECTOR, primary_count * asset_hash_probe.SECTOR))
        if 0 < stage_count <= 6:
            for stage in range(stage_count):
                sec_sector = asset_hash_probe.u16(data, base + 0x1E + stage * 2)
                sec_count = asset_hash_probe.u16(data, base + 0x2C + stage * 2)
                tert_sector = asset_hash_probe.u16(data, base + 0x3A + stage * 2)
                tert_count = asset_hash_probe.u16(data, base + 0x48 + stage * 2)
                if sec_count:
                    segments.append((f"secondary{stage}", sec_sector * asset_hash_probe.SECTOR, sec_count * asset_hash_probe.SECTOR))
                if tert_count:
                    segments.append((f"tertiary{stage}", tert_sector * asset_hash_probe.SECTOR, tert_count * asset_hash_probe.SECTOR))
        for segment, seg_off, seg_len in segments:
            toc = asset_hash_probe.valid_toc(data, seg_off, seg_len)
            if toc is None:
                continue
            for typ, size, rel in toc:
                if typ != 600:
                    continue
                abs_off = seg_off + rel
                if size < 4 or abs_off + 4 > len(data):
                    continue
                count = asset_hash_probe.u32(data, abs_off)
                if not (0 <= count < 2000) or 4 + count * 12 > size:
                    continue
                for entry_index in range(count):
                    entry_id, entry_size, entry_rel = struct.unpack_from("<III", data, abs_off + 4 + entry_index * 12)
                    if entry_rel + entry_size > size:
                        break
                    if entry_id in sprites:
                        continue
                    payload = data[abs_off + entry_rel:abs_off + entry_rel + entry_size]
                    entries.setdefault(entry_id, []).append((code, segment, payload))

    thumb_dir = (out_dir / "direct_blb_thumbnails").resolve()
    thumb_dir.mkdir(parents=True, exist_ok=True)
    added = 0
    for sprite_id, rows in sorted(entries.items()):
        image = None
        chosen_level = ""
        chosen_segment = ""
        for level, segment, payload in rows:
            image = decode_sprite_payload(payload)
            if image is not None:
                chosen_level = level
                chosen_segment = segment
                break
        if image is None:
            continue
        path = thumb_dir / f"sprite_{sprite_id}_direct.png"
        image.save(path, "PNG")
        rel = str(path.relative_to(ROOT))
        info = sprites.setdefault(sprite_id, SpriteInfo(sprite_id))
        info.levels.update(level for level, _segment, _payload in rows if level)
        info.segments.update(segment for _level, segment, _payload in rows if segment)
        info.images.append(SpriteImage(path, rel, chosen_level, chosen_segment, 0, None, image.size, alpha_bbox_area(image)))
        added += 1
    return added


def hash_neighborhood_ids(
    sprites: dict[int, SpriteInfo],
    regional_replacements: dict[int, tuple[int, int]],
    numeric_neighbors: int = 3,
    hamming_neighbors: int = 2,
) -> list[int]:
    """Return a focused, hash-sorted chunk around the regional suspects.

    Numeric adjacency in this sparse hash is only a weak clue, but it is useful
    for a quick visual pass. We also add a few nearest Hamming-distance ids for
    each regional id because one-character / short-suffix changes are more
    likely to preserve sparse-bit similarity than plain integer ordering.
    """
    sorted_ids = sorted(sprites)
    index = {sprite_id: i for i, sprite_id in enumerate(sorted_ids)}
    selected: set[int] = set()
    for sprite_id in sorted(regional_replacements):
        if sprite_id not in sprites:
            continue
        i = index[sprite_id]
        selected.update(sorted_ids[max(0, i - numeric_neighbors):min(len(sorted_ids), i + numeric_neighbors + 1)])
        nearest = sorted(
            (other for other in sorted_ids if other != sprite_id),
            key=lambda other: ((sprite_id ^ other).bit_count(), abs(sprite_id - other), other),
        )[:hamming_neighbors]
        selected.update(nearest)
    return sorted(selected)


def load_font(size: int) -> ImageFont.ImageFont:
    for font_path in [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf",
    ]:
        try:
            return ImageFont.truetype(font_path, size)
        except Exception:
            pass
    return ImageFont.load_default()


def fit_image(image: Image.Image, max_size: tuple[int, int]) -> Image.Image:
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is not None:
        rgba = rgba.crop(bbox)
    rgba.thumbnail(max_size, Image.Resampling.LANCZOS)
    return rgba


def draw_wrapped(draw: ImageDraw.ImageDraw, xy: tuple[int, int], text: str, font: ImageFont.ImageFont, fill, width_chars: int, max_lines: int) -> None:
    words = text.replace(",", ", ").split()
    lines: list[str] = []
    cur = ""
    for word in words:
        candidate = word if not cur else cur + " " + word
        if len(candidate) <= width_chars:
            cur = candidate
        else:
            if cur:
                lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    for i, line in enumerate(lines[:max_lines]):
        draw.text((xy[0], xy[1] + i * (font.size + 2)), line, font=font, fill=fill)


def draw_tile(
    sheet: Image.Image,
    draw: ImageDraw.ImageDraw,
    info: SpriteInfo,
    x: int,
    y: int,
    tile_w: int,
    tile_h: int,
    thumb_box: tuple[int, int],
    title_font: ImageFont.ImageFont,
    small_font: ImageFont.ImageFont,
    highlight: bool = False,
    extra_lines: Iterable[str] = (),
) -> None:
    bg = (255, 250, 210) if highlight else (250, 250, 250)
    outline = (220, 120, 30) if highlight else (180, 180, 180)
    draw.rounded_rectangle((x, y, x + tile_w - 1, y + tile_h - 1), radius=8, fill=bg, outline=outline, width=2 if highlight else 1)

    best = info.best_image()
    if best is not None:
        try:
            with Image.open(best.path) as image:
                thumb = fit_image(image, thumb_box)
            tx = x + (tile_w - thumb.width) // 2
            ty = y + 8 + (thumb_box[1] - thumb.height) // 2
            # checker-ish white panel behind transparent sprites
            draw.rectangle((x + 8, y + 8, x + tile_w - 9, y + 8 + thumb_box[1]), fill=(238, 238, 238), outline=(210, 210, 210))
            sheet.alpha_composite(thumb, (tx, ty))
        except Exception:
            pass

    text_y = y + thumb_box[1] + 18
    draw.text((x + 8, text_y), info.hex, font=title_font, fill=(10, 10, 10))
    text_y += title_font.size + 4
    levels = ", ".join(sorted(info.levels)) or "?"
    segments = ", ".join(sorted(info.segments)) or "?"
    draw_wrapped(draw, (x + 8, text_y), f"levels: {levels}", small_font, (40, 40, 40), 32, 2)
    text_y += (small_font.size + 2) * 2
    draw_wrapped(draw, (x + 8, text_y), f"segments: {segments}", small_font, (40, 40, 40), 32, 2)
    text_y += (small_font.size + 2) * 2
    draw.text((x + 8, text_y), f"files: {len(info.images)}", font=small_font, fill=(60, 60, 60))
    text_y += small_font.size + 3
    for line in extra_lines:
        draw.text((x + 8, text_y), line, font=small_font, fill=(120, 50, 0))
        text_y += small_font.size + 3


def write_contact_sheets(
    sprites: dict[int, SpriteInfo],
    out_dir: Path,
    ids: list[int],
    prefix: str,
    title: str,
    regional_replacements: dict[int, tuple[int, int]],
    columns: int = 5,
    rows: int = 5,
    tile_w: int = 220,
    tile_h: int = 220,
    highlight_ids: set[int] | None = None,
) -> list[Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    title_font = load_font(18)
    small_font = load_font(12)
    header_font = load_font(26)
    thumb_box = (tile_w - 20, 108)
    margin = 18
    header_h = 54
    page_size = columns * rows
    paths: list[Path] = []
    highlight_ids = highlight_ids or set()

    for page_index in range(math.ceil(len(ids) / page_size)):
        page_ids = ids[page_index * page_size:(page_index + 1) * page_size]
        width = margin * 2 + columns * tile_w
        height = margin * 2 + header_h + rows * tile_h
        sheet = Image.new("RGBA", (width, height), (255, 255, 255, 255))
        draw = ImageDraw.Draw(sheet)
        draw.text((margin, margin), f"{title} — page {page_index + 1}/{math.ceil(len(ids) / page_size)}", font=header_font, fill=(0, 0, 0))
        draw.text((margin, margin + 30), "Fill docs/analysis/asset-identification/sprite_identification_template.csv with role/name notes.", font=small_font, fill=(80, 80, 80))
        for i, sprite_id in enumerate(page_ids):
            info = sprites[sprite_id]
            col = i % columns
            row = i // columns
            x = margin + col * tile_w
            y = margin + header_h + row * tile_h
            extras = []
            if sprite_id in regional_replacements:
                fr, de = regional_replacements[sprite_id]
                extras = [f"FR: 0x{fr:08x}", f"DE: 0x{de:08x}"]
            draw_tile(sheet, draw, info, x, y, tile_w - 10, tile_h - 10, thumb_box, title_font, small_font, sprite_id in highlight_ids, extras)
        out_path = out_dir / f"{prefix}_{page_index + 1:02d}.png"
        sheet.convert("RGB").save(out_path, optimize=True)
        paths.append(out_path)
    return paths


def write_hash_cluster_sheets(
    sprites: dict[int, SpriteInfo],
    out_dir: Path,
    regional_replacements: dict[int, tuple[int, int]],
    clusters_per_page: int = 4,
) -> list[Path]:
    """Write grouped sheets: one row per regional id plus hash-near peers."""
    out_dir.mkdir(parents=True, exist_ok=True)
    sorted_ids = sorted(sprites)
    index = {sprite_id: i for i, sprite_id in enumerate(sorted_ids)}
    anchors = [sprite_id for sprite_id in sorted(regional_replacements) if sprite_id in sprites]
    title_font = load_font(14)
    small_font = load_font(10)
    header_font = load_font(24)
    tile_w = 170
    tile_h = 200
    thumb_box = (tile_w - 20, 86)
    columns = 7
    margin = 16
    header_h = 68
    paths: list[Path] = []

    for page_index in range(math.ceil(len(anchors) / clusters_per_page)):
        page_anchors = anchors[page_index * clusters_per_page:(page_index + 1) * clusters_per_page]
        width = margin * 2 + columns * tile_w
        height = margin * 2 + header_h + clusters_per_page * tile_h
        sheet = Image.new("RGBA", (width, height), (255, 255, 255, 255))
        draw = ImageDraw.Draw(sheet)
        draw.text((margin, margin), f"Regional hash clusters — page {page_index + 1}/{math.ceil(len(anchors) / clusters_per_page)}", font=header_font, fill=(0, 0, 0))
        draw.text((margin, margin + 30), "Each row: anchor, numeric neighbors, then nearest Hamming-distance neighbors.", font=small_font, fill=(80, 80, 80))

        for row, anchor in enumerate(page_anchors):
            anchor_index = index[anchor]
            numeric = sorted_ids[max(0, anchor_index - 2):anchor_index] + sorted_ids[anchor_index + 1:min(len(sorted_ids), anchor_index + 3)]
            hamming = sorted(
                (other for other in sorted_ids if other != anchor),
                key=lambda other: ((anchor ^ other).bit_count(), abs(anchor - other), other),
            )[:2]
            cluster = [anchor]
            for sprite_id in numeric + hamming:
                if sprite_id not in cluster:
                    cluster.append(sprite_id)
            y = margin + header_h + row * tile_h
            draw.text((margin, y + 2), f"center {anchor:#010x}", font=small_font, fill=(80, 80, 80))
            for col, sprite_id in enumerate(cluster[:columns]):
                x = margin + col * tile_w
                extras: list[str] = []
                highlight = sprite_id == anchor or sprite_id in regional_replacements
                if sprite_id == anchor:
                    fr, de = regional_replacements[sprite_id]
                    extras = ["ANCHOR", f"FR 0x{fr:08x}", f"DE 0x{de:08x}"]
                elif sprite_id in numeric:
                    extras = ["numeric-near"]
                else:
                    extras = [f"ham {(anchor ^ sprite_id).bit_count()}"]
                draw_tile(sheet, draw, sprites[sprite_id], x, y + 14, tile_w - 8, tile_h - 18, thumb_box, title_font, small_font, highlight, extras)

        out_path = out_dir / f"sprites_hash_clusters_{page_index + 1:02d}.png"
        sheet.convert("RGB").save(out_path, optimize=True)
        paths.append(out_path)
    return paths


def write_csv_template(sprites: dict[int, SpriteInfo], out_path: Path, regional_replacements: dict[int, tuple[int, int]]) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    existing: dict[str, dict[str, str]] = {}
    if out_path.exists():
        with out_path.open(newline="") as f:
            for row in csv.DictReader(f):
                if row.get("sprite_hex"):
                    existing[row["sprite_hex"].lower()] = row
    with out_path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "sprite_hex",
            "sprite_decimal",
            "regional_fr_hex",
            "regional_de_hex",
            "levels",
            "segments",
            "representative_png",
            "file_count",
            "human_role",
            "possible_original_name",
            "confidence",
            "notes",
        ])
        for sprite_id in sorted(sprites):
            info = sprites[sprite_id]
            best = info.best_image()
            fr, de = regional_replacements.get(sprite_id, ("", ""))
            old = existing.get(f"0x{sprite_id:08x}", {})
            writer.writerow([
                f"0x{sprite_id:08x}",
                sprite_id,
                f"0x{fr:08x}" if fr else "",
                f"0x{de:08x}" if de else "",
                ";".join(sorted(info.levels)),
                ";".join(sorted(info.segments)),
                best.rel if best else "",
                len(info.images),
                old.get("human_role", ""),
                old.get("possible_original_name", ""),
                old.get("confidence", ""),
                old.get("notes", ""),
            ])


def write_index(
    out_dir: Path,
    focused_pages: list[Path],
    cluster_pages: list[Path],
    regional_pages: list[Path],
    all_pages: list[Path],
    csv_path: Path,
    sprite_count: int,
    focused_count: int,
    direct_added: int,
) -> None:
    lines = [
        "# Sprite Asset Identification Contact Sheets",
        "",
        f"Generated from extracted sprite PNG/GIFs plus direct BLB thumbnail fallback. Unique sprite IDs: **{sprite_count}**.",
        f"Focused hash-neighborhood chunk: **{focused_count}** sprite IDs.",
        f"Direct BLB thumbnails added for extraction misses: **{direct_added}**.",
        "",
        "Use these sheets to identify sprites by visual role. Put identifications in the CSV template; do not rename hashes directly until a name/hash hypothesis is validated.",
        "",
        "Hash proximity note: plain integer adjacency is only a weak clue for this sparse bit-toggle hash. The focused sheet is centered on the PAL localized replacement IDs and also includes close Hamming-distance neighbors, which is a better first-pass similarity signal.",
        "",
        "## Fill-in template",
        "",
        f"- [{csv_path.name}]({csv_path.name})",
        "",
        "Columns to fill:",
        "",
        "- `human_role`: what the asset visibly is, e.g. `Klaymen head 1-up`, `Monkey Mage body`, `menu button`.",
        "- `possible_original_name`: only if there is a plausible ToolX/source-style filename guess.",
        "- `confidence`: `visual`, `runtime-confirmed`, `manual-name`, etc.",
        "",
        "## Focused hash-neighborhood chunk",
        "",
    ]
    for path in focused_pages:
        lines.append(f"- [{path.name}]({path.name})")
    lines += [
        "",
        "## Per-ID hash clusters",
        "",
        "Each row is centered on one regional suspect and shows nearby numeric-hash neighbors plus nearest low-Hamming-distance neighbors.",
        "",
    ]
    for path in cluster_pages:
        lines.append(f"- [{path.name}]({path.name})")
    lines += [
        "",
        "## Regional replacement suspects",
        "",
    ]
    for path in regional_pages:
        lines.append(f"- [{path.name}]({path.name})")
    if all_pages:
        lines += ["", "## All sprite IDs", ""]
    for path in all_pages:
        lines.append(f"- [{path.name}]({path.name})")
    lines.append("")
    (out_dir / "README.md").write_text("\n".join(lines))


def main() -> int:
    parser = argparse.ArgumentParser(description="Build labeled contact sheets for extracted Skullmonkeys sprite IDs")
    parser.add_argument("--extracted", type=Path, default=ROOT / "extracted", help="extracted BLB directory")
    parser.add_argument("--out", type=Path, default=ROOT / "docs/analysis/asset-identification", help="output directory")
    parser.add_argument("--columns", type=int, default=5)
    parser.add_argument("--rows", type=int, default=5)
    parser.add_argument("--write-all", action="store_true", help="also write paged sheets for every sprite id")
    args = parser.parse_args()

    if not args.extracted.is_dir():
        print(f"missing extracted dir: {args.extracted}", file=sys.stderr)
        return 1

    sprites = scan_sprites(args.extracted)
    if not sprites:
        print("no sprite PNGs found", file=sys.stderr)
        return 1

    out_dir = args.out
    out_dir.mkdir(parents=True, exist_ok=True)
    regional_replacements = load_regional_replacements()
    direct_added = add_missing_blb_thumbnails(sprites, out_dir)
    all_ids = sorted(sprites)
    regional_ids = [sprite_id for sprite_id in sorted(regional_replacements) if sprite_id in sprites]
    focused_ids = hash_neighborhood_ids(sprites, regional_replacements)

    focused_pages = write_contact_sheets(
        sprites, out_dir, focused_ids, "sprites_focused_hash_neighborhood", "Focused regional hash-neighborhood sprite IDs",
        regional_replacements,
        columns=args.columns, rows=args.rows, highlight_ids=set(regional_replacements),
    )
    regional_pages = write_contact_sheets(
        sprites, out_dir, regional_ids, "sprites_regional", "PAL regional replacement sprite IDs",
        regional_replacements,
        columns=args.columns, rows=args.rows, highlight_ids=set(regional_replacements),
    )
    cluster_pages = write_hash_cluster_sheets(sprites, out_dir, regional_replacements)
    all_pages: list[Path] = []
    if args.write_all:
        all_pages = write_contact_sheets(
            sprites, out_dir, all_ids, "sprites_all", "All extracted sprite IDs",
            regional_replacements,
            columns=args.columns, rows=args.rows, highlight_ids=set(regional_replacements),
        )
    csv_path = out_dir / "sprite_identification_template.csv"
    write_csv_template(sprites, csv_path, regional_replacements)
    write_index(out_dir, focused_pages, cluster_pages, regional_pages, all_pages, csv_path, len(sprites), len(focused_ids), direct_added)

    print(f"sprites: {len(sprites)}")
    print(f"regional ids present: {len(regional_ids)}")
    print(f"direct thumbnails added: {direct_added}")
    print(f"focused ids: {len(focused_ids)}")
    print(f"wrote: {out_dir / 'README.md'}")
    print(f"wrote: {csv_path}")
    print(f"focused pages: {len(focused_pages)}")
    print(f"cluster pages: {len(cluster_pages)}")
    print(f"regional pages: {len(regional_pages)}")
    print(f"all pages: {len(all_pages)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
