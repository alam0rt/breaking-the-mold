# Session 26 — ToolX: confirmed dev account (asset pipeline + naming model)

First-person dev-team account of the asset pipeline (previously this quote was flagged
UNVERIFIED in the original digest — now confirmed as a real dev statement).

## The quote (asset pipeline, end to end)
"...sequentially numbered bitmap files (output from the Sony Capture software) ... loaded into
Debabelizer Pro, where we chroma-keyed out the greenscreen. We then ... built our sequences
using our proprietary animation scripting software, **ToolX**, written by **Kenton Leach**.
Using this tool was similar to entering the images into an **exposure sheet**, which then wrote
out a **proprietary sequence file** read by our game engine (written by Kenton Leach, Tim
Lorenzen, Brian Bellfield). Rather than a programmer ... ToolX allows our ANIMATORS to have
complete control of how their animation looks."

## Confirms (upgrades earlier "unverified" status)
- **ToolX is real**, named correctly, authored by **Kenton Leach**. Engine: Leach + Tim
  Lorenzen + Brian Bellfield. (Was folklore/unconfirmed in the digest; now a dev account.)
- **Pipeline**: Sony Capture (sequential numbered BMPs) -> Debabelizer Pro (greenscreen key)
  -> ToolX (exposure sheet -> sequence file) -> engine.

## Validates our reverse-engineering (independent agreement)
- **Asset 503 = "ToolX animation sequence data"** (Session 15) IS the exposure-sheet output
  ("proprietary sequence file"). Why sequences are u16 INDICES (FUN_800255c8): an exposure
  sheet is an ordered list of frame indices.
- "Sequentially numbered bitmap files" = the frame runs we see as radius-<=3 sister-clusters
  (the frames of one animation). The hashed sprite ID keys the sequence; frames are ordered
  within it.

## Refines the NAMING model (important)
- The hashed names are **animator-typed sequence labels in ToolX's exposure sheet** — NOT the
  capture filenames (those were sequential numbers like walk0001.bmp) and NOT character
  nicknames. This explains why every recovered name is a FUNCTIONAL/DESCRIPTIVE label
  (WIZARD_ATTACK, KLAY_RUN_FAST, FX_OBJECT_HAMSTER, STATUSNUMBERS) chosen by an animator
  describing the animation — not creative/marketing names.
- Corollary (tested this session): interview CHARACTER nicknames (e.g. "Slappy" for the
  hamster) are the WRONG vocabulary — the asset namer used the functional word (HAMSTER:
  verified as both <PREFIX>HAMSTER pickup 0xA9228088 and FX_OBJECT_HAMSTER 0x22A4AA42). So
  character codenames are low-value as a crack wordlist; functional/animation terms are what hit.
- The FX_/category prefixes (FX_, _OBJECT_, _BOSS_, _KLAY_) are likely ToolX's exposure-sheet
  ORGANIZATIONAL groups/folders, prepended to the animator's label.

## "Slappy" test result (negative, recorded)
Tested SLAPPY + variants x actions x 4 namespaces (~102k candidates): only hit was a contrived
18-char string on an AUDIO id with nonsensical ANIM_ prefix (expected ~0.06 random hits ->
noise, REJECTED). The hamster's assets use "HAMSTER", not "Slappy". Confirms the model above.

## Net
- ToolX dev account confirmed; ties our Asset-503 / sequence-index findings to the actual tool.
- Naming model sharpened: animator-chosen functional sequence labels + ToolX category prefix.
  Character nicknames don't match the asset vocabulary -> deprioritize as wordlist.
