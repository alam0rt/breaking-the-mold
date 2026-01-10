"""
BLB Asset Extraction Tool

Extracts all assets from a Skullmonkeys GAME.BLB file using the ImHex JSON output
to locate assets, then reads raw bytes from the BLB binary.

Usage:
    python -m tools.extract_blb --json /tmp/blb.json --blb disks/blb/GAME.BLB -o extracted/
"""
