# Sprite Asset Identification Contact Sheets

Generated from extracted sprite PNG/GIFs plus direct BLB thumbnail fallback. Unique sprite IDs: **412**.
Focused hash-neighborhood chunk: **86** sprite IDs.
Direct BLB thumbnails added for extraction misses: **23**.

Use these sheets to identify sprites by visual role. Put identifications in the CSV template; do not rename hashes directly until a name/hash hypothesis is validated.

Hash proximity note: plain integer adjacency is only a weak clue for this sparse bit-toggle hash. The focused sheet is centered on the PAL localized replacement IDs and also includes close Hamming-distance neighbors, which is a better first-pass similarity signal.

## Fill-in template

- [sprite_identification_template.csv](sprite_identification_template.csv)

Columns to fill:

- `human_role`: what the asset visibly is, e.g. `Klaymen head 1-up`, `Monkey Mage body`, `menu button`.
- `possible_original_name`: only if there is a plausible ToolX/source-style filename guess.
- `confidence`: `visual`, `runtime-confirmed`, `manual-name`, etc.

## Focused hash-neighborhood chunk

- [sprites_focused_hash_neighborhood_01.png](sprites_focused_hash_neighborhood_01.png)
- [sprites_focused_hash_neighborhood_02.png](sprites_focused_hash_neighborhood_02.png)
- [sprites_focused_hash_neighborhood_03.png](sprites_focused_hash_neighborhood_03.png)
- [sprites_focused_hash_neighborhood_04.png](sprites_focused_hash_neighborhood_04.png)

## Per-ID hash clusters

Each row is centered on one regional suspect and shows nearby numeric-hash neighbors plus nearest low-Hamming-distance neighbors.

- [sprites_hash_clusters_01.png](sprites_hash_clusters_01.png)
- [sprites_hash_clusters_02.png](sprites_hash_clusters_02.png)
- [sprites_hash_clusters_03.png](sprites_hash_clusters_03.png)
- [sprites_hash_clusters_04.png](sprites_hash_clusters_04.png)
- [sprites_hash_clusters_05.png](sprites_hash_clusters_05.png)
- [sprites_hash_clusters_06.png](sprites_hash_clusters_06.png)

## Regional replacement suspects

- [sprites_regional_01.png](sprites_regional_01.png)
