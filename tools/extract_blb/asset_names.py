"""
Known asset-name lookup for BLB extraction.

Maps a 32-bit asset hash id (sound id, sprite id, ...) to its human-readable
name when one has been reverse-engineered. The source of truth is the
clean-room name catalogue at:

    docs/analysis/asset-identification/cracked_names.csv

Columns: id_hex,id_dec,floor,type,levels,status,name,method,alts

Only rows whose ``status`` is ``verified`` (and which carry a non-empty
``name``) are returned. Uncracked guesses and weaker anchors (e.g.
``scummvm_anchor``) are ignored, so the extractor never invents a label or
claims more certainty than the catalogue has — those ids fall back to the
raw hash.

Clean-room caveat: every name here is an *inferred* label, not an original
symbol. Treat filenames as hypotheses backed by the catalogue's evidence.
"""

import csv
import os
from functools import lru_cache
from pathlib import Path

# Repo root: tools/extract_blb/asset_names.py -> parents[2]
_REPO_ROOT = Path(__file__).resolve().parents[2]
_DEFAULT_CSV = _REPO_ROOT / "docs" / "analysis" / "asset-identification" / "cracked_names.csv"


def _csv_path() -> Path:
    """Catalogue path, overridable via SKULLMONKEYS_ASSET_NAMES for tests."""
    override = os.environ.get("SKULLMONKEYS_ASSET_NAMES")
    return Path(override) if override else _DEFAULT_CSV


@lru_cache(maxsize=None)
def _load(csv_path_str: str) -> dict[tuple[int, str], str]:
    """
    Load the catalogue into {(asset_id, type): name}.

    Keyed by (id, type) so the same hash catalogued under different asset
    types stays disambiguated; lookups may also ignore type (see lookup_name).
    Returns an empty map if the catalogue is missing or unreadable.
    """
    path = Path(csv_path_str)
    mapping: dict[tuple[int, str], str] = {}
    if not path.exists():
        return mapping

    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            # Only trust fully verified names; skip uncracked guesses and
            # weaker anchors (e.g. scummvm_anchor) so filenames never claim
            # more certainty than the catalogue has.
            if (row.get("status") or "").strip() != "verified":
                continue
            name = (row.get("name") or "").strip()
            id_hex = (row.get("id_hex") or "").strip()
            if not name or not id_hex:
                continue
            try:
                asset_id = int(id_hex, 16)
            except ValueError:
                continue
            mapping[(asset_id, (row.get("type") or "").strip())] = name
    return mapping


def lookup_name(asset_id: int, asset_type: str | None = None) -> str | None:
    """
    Return the known name for ``asset_id``, or None if unknown.

    If ``asset_type`` is given (e.g. "audio"), only a catalogue entry of that
    type matches; otherwise the first entry for the id (any type) is returned.
    """
    mapping = _load(str(_csv_path()))
    if asset_type is not None:
        return mapping.get((asset_id, asset_type))
    for (cid, _ctype), name in mapping.items():
        if cid == asset_id:
            return name
    return None


def sanitize_stem(name: str) -> str:
    """Make a catalogue name safe as a filename stem (e.g. "QUIT GAME" -> "QUIT_GAME")."""
    return "".join(c if (c.isalnum() or c in "._-") else "_" for c in name)


def known_stem(asset_id: int, asset_type: str, fallback: str) -> str:
    """
    Filename stem for an asset: its sanitised known name, or ``fallback``.

    This is the single place handlers go to turn an id into a file stem, so the
    "prefer real name, else keep the hash" policy stays identical across asset
    types. ``fallback`` is the caller's hash-based default (it is used verbatim,
    so callers stay in control of their own hash format).
    """
    name = lookup_name(asset_id, asset_type)
    return sanitize_stem(name) if name else fallback
