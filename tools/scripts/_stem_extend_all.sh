#!/usr/bin/env bash
# Run kcrack extend over every known cracked family stem.
# For each stem, brute up to 5 alphanumeric chars suffix + 0-2 prefix chars.
# Look for hits in uncracked audio target list.

set -e
cd "$(dirname "$0")/../.."

TARGETS=/tmp/targets_audio.txt
OUT=/tmp/stem_extend_hits.txt
> "$OUT"

# Every known FX_ family stem (extracted from cracked_names.csv verified rows).
# Format: stem trailing the underscore, so 'FX_KLAY_' brute appends ACTION suffix.
STEMS=(
    # Player
    'FX_KLAY_'
    'FX_KLAY_FOOTSTEP_'
    'FX_KLAY_DUCK_'
    'FX_KLAY_RUN_'
    'FX_KLAY_DIE_'
    'FX_KLAY_LAND_'
    # Skull bot
    'FX_SKULL_'
    'FX_SKULL_FOOTSTEP_'
    'FX_SKULL_LAND_'
    'FX_SKULL_SPRING_'
    'FX_SKULL_FIRE_'
    'FX_SKULL_FLY_'
    'FX_SKULL_SCREAM_'
    'FX_SKULL_GRUNT_'
    # Boss families
    'FX_BOSS_HEAD_'
    'FX_BOSS_HEAD_IDLE_'
    # Pickups
    'FX_PICKUP_'
    'FX_PICKUP_1970_'
    'FX_PICKUP_PHOENIX_'
    'FX_PICKUP_HAMSTER_'
    'FX_PICKUP_GLIDEY_'
    'FX_PICKUP_FARTHEAD_'
    'FX_PICKUP_SHIELD_'
    'FX_PICKUP_GROW_'
    'FX_PICKUP_ONE_UP_'
    'FX_PICKUP_SUPER_WILLIE_'
    'FX_PICKUP_UNIVERSE_ENEMA_'
    # Animals
    'FX_BIRD_'
    'FX_BIRD_FLY_'
    'FX_BIRD_SQUISH_'
    'FX_RAT_'
    'FX_RAT_DASH_'
    'FX_GUM_'
    'FX_GUM_PIERCE_'
    'FX_PUFF_'
    'FX_PUFF_FALL_'
    'FX_FINN_'
    'FX_FINN_DIE_'
    # Menu
    'FX_MENU_'
)

for stem in "${STEMS[@]}"; do
    echo "=== ${stem} ===" | tee -a "$OUT"
    ./tools/kcrack extend "$stem" "$TARGETS" --raw --suf 5 --pre 0 2>/dev/null | tee -a "$OUT"
done

echo
echo "Total hits:"
grep -c '^0x' "$OUT" || true
echo "See $OUT"
