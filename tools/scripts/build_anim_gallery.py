#!/usr/bin/env python3
"""Build animated-GIF gallery of uncracked sprites for naming review.

Assembles a per-sprite animated GIF from its extracted frame PNGs (or copies an
existing anim00.gif), scaled small, and writes a mobile-friendly anim.html grid
captioned with id / levels / role. Sprites are ordered known-role & entity first,
blank filler tiles last. Output goes straight into the share dir.

Run: python3 tools/scripts/build_anim_gallery.py [--out /mnt/share/public/www/klay] [--max 200]
"""
from __future__ import annotations
import argparse, csv, re
from collections import defaultdict
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
EXT = ROOT / "extracted"
FRE = re.compile(r"sprite_(\d+)_anim00(?:_f(\d+))?\.(png|gif)$")


def load_info():
    info = {}
    for r in csv.DictReader((AID / "cracked_names.csv").open()):
        if r["id_hex"].startswith("0x") and r["status"] == "uncracked" and r["type"] in ("sprite", "anim"):
            info[int(r["id_hex"], 16)] = {"levels": r["levels"], "type": r["type"], "role": "", "caller": ""}
    tpl = AID / "sprite_identification_template.csv"
    if tpl.exists():
        for r in csv.DictReader(tpl.open()):
            try: idv = int(r["sprite_hex"], 16)
            except Exception: continue
            if idv in info and r.get("human_role"): info[idv]["role"] = r["human_role"]
    coh = AID / "asset_cohorts.csv"
    if coh.exists():
        for r in csv.DictReader(coh.open()):
            caller = (r.get("callers") or "").split(";")[0]
            for m in (r.get("members") or "").split(";"):
                if m.strip().startswith("0x"):
                    idv = int(m, 16)
                    if idv in info and not info[idv]["caller"]: info[idv]["caller"] = caller
    # brute candidate name guesses
    hits = AID / "brute_review_hits.csv"
    if hits.exists():
        for r in csv.DictReader(hits.open()):
            idv = int(r["id_hex"], 16)
            if idv in info:
                info[idv].setdefault("guesses", [])
                g = (r["model"], r["candidate"])
                if g not in info[idv]["guesses"]:
                    info[idv]["guesses"].append(g)
    return info


def frames_for(idv: int):
    """Return ordered list of frame PNG paths from one source dir (prefer 'primary')."""
    dec = str(idv)
    by_dir = defaultdict(list)
    for p in EXT.glob(f"*/*/sprites/sprite_{dec}_anim00_f*.png"):
        by_dir[p.parent].append(p)
    if not by_dir:
        single = list(EXT.glob(f"*/*/sprites/sprite_{dec}_anim00.png"))
        return single[:1]
    def score(d): return (0 if "primary" in str(d) else 1, str(d))
    best = sorted(by_dir, key=score)[0]
    return sorted(by_dir[best], key=lambda p: int(FRE.search(p.name).group(2) or 0))


def is_blankish(im: Image.Image) -> bool:
    cols = im.convert("RGB").getcolors(maxcolors=4096)
    if cols is None: return False
    return len({c for _, c in cols}) <= 2


def build_gif(idv: int, paths, out: Path, box=128):
    imgs = []
    for p in paths:
        im = Image.open(p).convert("RGBA")
        bg = Image.new("RGBA", im.size, (32, 32, 40, 255)); bg.alpha_composite(im)
        im = bg.convert("RGB"); im.thumbnail((box, box), Image.NEAREST)
        c = Image.new("RGB", (box, box), (24, 24, 28)); c.paste(im, ((box - im.width) // 2, (box - im.height) // 2))
        imgs.append(c)
    if not imgs: return None, False
    blank = is_blankish(imgs[0])
    op = out / f"a_{idv:08x}.gif"
    if op.exists():
        return op.name, blank
    if len(imgs) == 1:
        imgs[0].save(op)
    else:
        imgs[0].save(op, save_all=True, append_images=imgs[1:], duration=120, loop=0, optimize=True)
    return op.name, blank


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="/mnt/share/public/www/klay")
    ap.add_argument("--box", type=int, default=128)
    args = ap.parse_args()
    out = Path(args.out); out.mkdir(parents=True, exist_ok=True)
    info = load_info()
    order = sorted(info, key=lambda i: (info[i]["role"] == "", info[i]["caller"] == "", info[i]["levels"], i))

    cards = []
    for idv in order:
        fr = frames_for(idv)
        if not fr: continue
        name, blank = build_gif(idv, fr, out, args.box)
        if not name: continue
        cards.append((idv, name, len(fr), blank))
    # cards WITH name guesses first, then non-blank role/entity-known, blanks last
    cards.sort(key=lambda c: (not info[c[0]].get("guesses"), c[3],
                              info[c[0]]["role"] == "", info[c[0]]["caller"] == ""))

    rows = []
    for idv, name, nf, blank in cards:
        m = info[idv]
        hint = m["role"] or m["caller"] or ""
        guesses = [g for g in m.get("guesses", []) if int(idv) != 0]
        gl = ""
        if guesses:
            spans = " ".join(f'<span class="{"gw" if mdl=="WRAP" else "gr"}">{c}</span>' for mdl, c in guesses[:12])
            gl = f'<br><span class=guess>{spans}</span>'
        rows.append(
            f'<figure class="{"bl" if blank else ""}"><img src="{name}" loading=lazy>'
            f'<figcaption><b>0x{idv:08x}</b> <span class=lv>{nf}f &middot; {m["levels"][:18]}</span>'
            f'{("<br><span class=role>"+hint+"</span>") if hint else ""}{gl}</figcaption></figure>')
    html = f"""<!doctype html><html><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>klay sprite animations</title>
<style>body{{background:#0b0b0d;color:#9be;font:12px/1.4 system-ui,monospace;margin:0;padding:8px}}
h1{{color:#9f9;font-size:16px}}a.b{{display:inline-block;margin:4px 6px 4px 0;padding:6px 11px;background:#14151b;border:1px solid #2a2e3c;border-radius:7px;color:#6af;text-decoration:none}}
.grid{{display:grid;grid-template-columns:repeat(auto-fill,minmax(132px,1fr));gap:8px}}
figure{{margin:0;background:#15151b;border:1px solid #222;border-radius:6px;padding:4px;text-align:center}}
figure.bl{{opacity:.45}}img{{width:128px;height:128px;image-rendering:pixelated}}
figcaption{{font-size:11px;color:#bcd}}b{{color:#cd6}}.lv{{color:#789}}.role{{color:#7df}}
.guess{{font-size:10px;word-break:break-all}}.guess .gr{{color:#6f9}}.guess .gw{{color:#f9a}}</style></head><body>
<h1>Sprite animations &mdash; {len([c for c in cards if not c[3]])} real + {len([c for c in cards if c[3]])} blank tiles</h1>
<a class=b href="sheets.html">contact sheets</a><a class=b href="guesses.html">name guesses</a>
<p style="color:#8aa;max-width:60ch">Full animation per uncracked sprite. Faded = blank/filler tile.
Recognise one? Send its <b>0x&hellip;</b> id + a name guess and I&rsquo;ll hash-check + probe siblings.</p>
<div class=grid>
{''.join(rows)}
</div></body></html>"""
    (out / "anim.html").write_text(html)
    print(f"built {len(cards)} gifs ({sum(1 for c in cards if not c[3])} real, {sum(1 for c in cards if c[3])} blank) -> {out}/anim.html")


if __name__ == "__main__":
    main()
