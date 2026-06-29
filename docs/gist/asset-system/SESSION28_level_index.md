---
title: "Session 28 — Level index map (level-data-context.md)"
category: gist/asset-system
tags: [gist, asset, system, level]
---

# Session 28 — Level index map (level-data-context.md)

## Index map (0-25)
0 MENU,1 GLEN,2 SCIE,3 CRYS,4 WEED,5 HEAD,6 BOIL,7 TMPL,8 CAVE,9 FOOD,10 CSTL,11 CLOU,12 PHRO,
13 WIZZ,14 BRG1,15 MOSS,16 SOAR,17 EGGS,18 FINN,19 GLID,20 KLOG,21 SNOW,22 EVIL,23 RUNN,24 MEGA,
25 SEVN.

## Asset-ID cracking: NEGATIVE (but informative)
The 4-char level codes are **NOT hashed asset names**: popcount-4, none appear in the id list,
and no code+prefix variant (FX_X, X_TITLE, X_BG, LEVEL_X, ...) hits any id in any namespace.
They are internal level identifiers/indices used as raw strings + this numeric index — never
run through calcHash to key an asset. (So level banners/titles, if they exist as sprites, are
named something else, not "GLEN"/"SCIE".)

## Where it IS useful
1. **Decodes the teleporter branch graph (Session 25):** branching teleporters write a
   destination LEVEL INDEX into GameState+0x19d (from teleporter entity +0x50), then
   SetupAndStartLevel(index). This table turns those opaque numbers into levels:
   dest 18=FINN, 25=SEVN (1970s bonus), 20=KLOG, etc. Once the per-teleporter +0x50 values are
   pulled from the BLB Asset-501 data, this map yields the full stage-to-stage branch graph.
2. **Decodes the debug level-select menu (Session 27):** ProcessDebugMenuInput indexes this
   table; pairs with g_LevelNameTable display strings.
3. **Validation:** exact 1:1 with the 26 levels in ids.csv (no discrepancy) — confirms our
   level-tag data is complete and correct.

## Structural insight (asset count by index)
Lowest counts = special stages: MENU 56 (UI only), FINN 62 / RUNN 68 (special-id-99 3D vehicle
RAIL stages — sparse by design), KLOG 65 / GLEN 68 (small/boss). Normal platformer levels
cluster 140-208 (TMPL 208 highest). Confirms the special-stage model from earlier sessions.

## Net
- Level codes are NOT in the hash namespace (clean negative for cracking).
- This index is the DECODER for teleporter destinations + debug-menu selection — closes the
  loop on the branching-teleporter work (Session 25).
- Cross-validates the 26-level tag set exactly.

## Files
- `level_index_map.json`
