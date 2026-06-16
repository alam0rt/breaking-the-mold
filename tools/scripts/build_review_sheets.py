#!/usr/bin/env python3
"""Build labeled contact sheets of uncracked sprites for human naming review.

For each uncracked sprite id: pick the best representative frame from extracted/,
draw a thumbnail with a caption (id hex, #levels, cohort-caller entity hint,
visual role if known). Tile into paginated grids so a human can recognize a
sprite and propose a name; names then get hash-checked with calcHash (RAW/WRAP).

Output: docs/analysis/asset-identification/review_sheets/sheet_NN.png
Run:    python3 tools/scripts/build_review_sheets.py [--per 48] [--cell 120]
"""
from __future__ import annotations
import argparse, csv, re
from collections import defaultdict
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
EXT = ROOT / "extracted"
OUT = AID / "review_sheets"
SPRITE_RE = re.compile(r"sprite_(\d+)_anim(\d+)(?:_f(\d+))?\.(png|gif)$")


def load_sprites():
    """uncracked sprite/anim ids -> dict(levels, nlev, role, caller)."""
    info = {}
    with (AID / "cracked_names.csv").open() as f:
        for r in csv.DictReader(f):
            if not r["id_hex"].startswith("0x"): continue
            if r["status"] != "uncracked": continue
            if r["type"] not in ("sprite", "anim"): continue
            lv = r["levels"]
            info[int(r["id_hex"], 16)] = {
                "levels": lv, "nlev": len([x for x in lv.split(";") if x]),
                "role": "", "caller": "",
            }
    # visual roles
    tpl = AID / "sprite_identification_template.csv"
    if tpl.exists():
        with tpl.open() as f:
            for r in csv.DictReader(f):
                try: idv = int(r["sprite_hex"], 16)
                except Exception: continue
                if idv in info and r.get("human_role"):
                    info[idv]["role"] = r["human_role"]
    # cohort caller (entity) hint
    coh = AID / "asset_cohorts.csv"
    if coh.exists():
        with coh.open() as f:
            for r in csv.DictReader(f):
                caller = (r.get("callers") or "").split(";")[0]
                for m in (r.get("members") or "").split(";"):
                    m = m.strip()
                    if m.startswith("0x"):
                        idv = int(m, 16)
                        if idv in info and not info[idv]["caller"]:
                            info[idv]["caller"] = caller
    return info


def find_image(idv: int) -> Path | None:
    """Best representative frame: prefer an animated gif, else a mid-index png."""
    dec = str(idv)
    cands = list(EXT.glob(f"*/*/sprites/sprite_{dec}_anim00.gif"))
    if cands:
        return cands[0]
    pngs = sorted(EXT.glob(f"*/*/sprites/sprite_{dec}_anim00_f*.png"))
    if pngs:
        return pngs[len(pngs) // 2]  # middle frame (often most distinctive)
    any_png = list(EXT.glob(f"*/*/sprites/sprite_{dec}_anim00.png"))
    return any_png[0] if any_png else None


def thumb(path: Path, box: int) -> Image.Image:
    im = Image.open(path)
    try: im.seek(im.n_frames // 2)  # mid gif frame
    except Exception: pass
    im = im.convert("RGBA")
    bg = Image.new("RGBA", im.size, (40, 40, 48, 255))
    bg.alpha_composite(im)
    im = bg.convert("RGB")
    im.thumbnail((box, box), Image.NEAREST)
    canvas = Image.new("RGB", (box, box), (24, 24, 28))
    canvas.paste(im, ((box - im.width) // 2, (box - im.height) // 2))
    return canvas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--per", type=int, default=48)
    ap.add_argument("--cell", type=int, default=120)
    ap.add_argument("--cols", type=int, default=8)
    args = ap.parse_args()

    info = load_sprites()
    # order: known role first, then sprites with a cohort/entity hint, then by level then id
    order = sorted(info, key=lambda i: (
        info[i]["role"] == "", info[i]["caller"] == "",
        info[i]["levels"], i))

    OUT.mkdir(parents=True, exist_ok=True)
    try: font = ImageFont.truetype("DejaVuSans.ttf", 9)
    except Exception: font = ImageFont.load_default()

    cell, cap = args.cell, 34
    cols, per = args.cols, args.per
    rows = (per + cols - 1) // cols
    pageW = cols * cell
    pageH = rows * (cell + cap)

    pages, missing = [], 0
    batch, page_no = [], 0
    def flush(batch, page_no):
        sheet = Image.new("RGB", (pageW, pageH), (16, 16, 20))
        d = ImageDraw.Draw(sheet)
        for k, idv in enumerate(batch):
            r, c = divmod(k, cols)
            x, y = c * cell, r * (cell + cap)
            img = find_image(idv)
            if img:
                try: sheet.paste(thumb(img, cell - 4), (x + 2, y + 2))
                except Exception: d.rectangle([x+2,y+2,x+cell-2,y+cell-2], outline=(90,40,40))
            else:
                d.rectangle([x+2,y+2,x+cell-2,y+cell-2], outline=(90,40,40))
                d.text((x+6, y+cell//2), "no img", font=font, fill=(150,80,80))
            meta = info[idv]
            cy = y + cell
            d.text((x+2, cy),    f"0x{idv:08x} L{meta['nlev']}", font=font, fill=(220,220,160))
            hint = meta["role"] or meta["caller"] or meta["levels"][:18]
            d.text((x+2, cy+11), hint[:24], font=font, fill=(150,200,230))
        op = OUT / f"sheet_{page_no:02d}.png"
        sheet.save(op)
        return op

    for idv in order:
        batch.append(idv)
        if len(batch) == per:
            pages.append(flush(batch, page_no)); page_no += 1; batch = []
    if batch:
        pages.append(flush(batch, page_no))

    print(f"sprites: {len(info)}  sheets: {len(pages)} -> {OUT.relative_to(ROOT)}")
    for p in pages: print("  ", p.relative_to(ROOT))


if __name__ == "__main__":
    main()
