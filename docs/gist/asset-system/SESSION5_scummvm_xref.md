# Session 5 — ScummVM / Neverhood cross-reference (external ground truth)

## Goal
The Neverhood (1996, PC) is the same studio's prequel on the **same engine + same calcHash**.
ScummVM reverse-engineered it with real hash constants in source. Any hash matching our
Skullmonkeys ids = the first EXTERNALLY-corroborated evidence (independent of our own RE).

## Method
Sparse-cloned `engines/neverhood` (139 files), harvested all 2,697 distinct 32-bit hex
constants, cross-matched against our 658 ids — directly (RAW) and under each namespace
transform (TEXT/PICKUP/BOSS).

## Foundational confirmation
ScummVM's `calcHash` (resource.cpp) is **byte-for-byte identical** to the function used
throughout this project (digit rule `+22`, shiftValue mod 32, single-bit XOR toggle).
Verified our Python reproduces it exactly on CONTINUE/FX_KLAY_LAND/WIZARDATTACK01/Klaymen/
STATUSNUMBERS. The entire analysis rests on a now-independently-validated hash.

## Matches (3 real; all in the RAW namespace)
| SK id | our label | ScummVM context | verdict |
|---|---|---|---|
| `0x5860C640` | FX_KLAY_LAND | `playSound(0,0x5860C640)` in Klaymen land code (4 sites) | **role-confirmed: Klaymen landing sound** |
| `0x9D406340` | Klaymen_idlehead_sound | inside `Klaymen::hmIdleHeadOff()` | **role-confirmed** |
| `0x5900C41E` | Klaymen_idleblink_anim | inside `Klaymen::stIdleBlink()` | **role-confirmed** |

(`0x00000000` also "matched" — a null sentinel, ignored.)

## Findings
- **First external ground truth in the project.** These 3 Klaymen assets carried over from
  The Neverhood and ScummVM's independent RE confirms their ROLE (what they do). Note:
  ScummVM also only knows the hashes, not the original strings — so this confirms role,
  not the literal ToolX name.
- **Klaymen's shared assets live in the RAW namespace** (direct calcHash, seed 0). Explains
  why BOSS/PICKUP seeds never decoded player assets.
- **0 NEW ids unlocked.** All 3 matches were already in our named set (from the earlier
  scummvm_xref). Skullmonkeys is a platformer vs Neverhood's point-and-click, so only core
  Klaymen sound/anim assets overlap.
- **0 matches under TEXT/PICKUP/BOSS** — those namespaces are Skullmonkeys-original
  (HUD/pickups/platformer-bosses didn't exist in Neverhood). Cleanly rules out false hits.
- Idle-sound neighbors ScummVM lists (`0x53A4A1D4`, `0x5BE0A3C6`, ...) are NOT in our id
  list — Skullmonkeys didn't ship those Neverhood sounds. Overlap is genuinely tiny.

## Verdict
The Neverhood xref is **exhausted as a source of new names** but delivered two things of
lasting value: (1) byte-exact validation of calcHash, and (2) external role-confirmation of
the 3 carried-over Klaymen assets. No further yield expected from this channel.
