# Skullmonkeys Asset Name Hunt

A static, single-page game for crowd-sourcing original asset names. It shows a
random sprite; you type a guess; the page hashes it client-side with the
recovered ToolX formula (`tools/klash.c` / `klash.js`) and turns **green** when
`asset_id(guess)` matches the sprite's baked-in id. Correct names persist to
`localStorage` and can be exported as JSON.

## Run

It's fully static — open `index.html` directly, or serve it:

```bash
python3 -m http.server -d tools/name-game 8000   # http://localhost:8000
```

## Files

- `index.html` — the game (UI + logic, inline).
- `klash.js` — the hash (`assetId`/`assetIdHex`), JS port of `tools/klash.c`, with a self-check.
- `manifest.js` — `window.MANIFEST`: one representative PNG + level hints per sprite id.
- `assets/` — the representative PNGs (one per id).
- `build_manifest.py` — regenerates `manifest.js` + `assets/` from `extracted/` and the id CSVs.

## Regenerate the manifest

After re-extracting sprites or updating the id list:

```bash
python3 tools/name-game/build_manifest.py
```

## Collecting answers

Each player's confirmed names live in their browser (`localStorage`
`skullmonkeys_names`). **Export JSON** downloads `[{hex,id,name}, …]`. Merge those
exports back into `docs/analysis/asset-identification/` to grow the verified
`name → id` corpus. The six already-cracked names (NO/YES/QUIT/CONTINUE/QUIT
GAME/PAUSED) are pre-seeded.

## Notes / extensions

- Only sprites are shown for now (audio/tiles are raw `.bin` in `extracted/` and
  need conversion to web-playable form first — a future `kind` in the manifest).
- The hash ignores case and non-alphanumerics, so `quit game`, `QUITGAME`, and
  `Quit_Game` all match.
