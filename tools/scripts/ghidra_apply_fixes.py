#!/usr/bin/env python3
"""
Apply approved naming/congruence fixes to the Skullmonkeys Ghidra program
(headless). Keep the Ghidra GUI CLOSED while running so the project lock
doesn't collide.

Fixes (from the 2026-06-14 audit):
  #1 RunnPlayerEntity +0x6e: byte bUnk108_writeOnly -> short velocityY
  #2 unify divergent shared sprite-block field names
  #6 delete tombstoned /game/ObsoleteSpriteTypeCallbackEntry

Run: nix develop -c python3 tools/scripts/ghidra_apply_fixes.py
"""
import sys
from pathlib import Path

PROJECT = "/mnt/share/sam/ghidra"
NAME = "skullmonkeys"
PROGRAM = "SLES_010.90"
PSX_EXT = "/nix/store/z088mpgaxbk78rvr4c011pbk2k50ag2p-ghidra-psx-ldr-2025.09.06/lib/ghidra/Ghidra/Extensions/ghidra-psx-ldr"

# (struct path, offset, new field name)
RENAMES = [
    ("/Skullmonkeys/SpriteEntity", 0x8a, "texPageByte"),
    ("/Skullmonkeys/SpriteEntity", 0x98, "nextStateMarker"),
    ("/Skullmonkeys/SpriteEntity", 0x9c, "nextStateCallback"),
    ("/Skullmonkeys/PlayerEntity", 0xa0, "activeStateMarker"),
    ("/Skullmonkeys/PlayerEntity", 0xa8, "exitCallbackMarker"),
    ("/Skullmonkeys/PlayerEntity", 0xe2, "sequenceStep"),
    ("/Skullmonkeys/PlayerEntity", 0xe4, "sequenceLength"),
]
# (struct path, offset, new short field name, comment)
RETYPE_SHORT = [
    ("/Skullmonkeys/RunnPlayerEntity", 0x6e, "velocityY", "Y velocity"),
]
DELETE_TYPES = ["/game/ObsoleteSpriteTypeCallbackEntry"]


def log(*a):
    print(*a, file=sys.stderr, flush=True)


def main():
    import pyghidra
    from pyghidra import HeadlessPyGhidraLauncher, ExtensionDetails

    launcher = HeadlessPyGhidraLauncher(verbose=True)
    if Path(PSX_EXT).exists():
        launcher.install_plugin(Path(PSX_EXT), ExtensionDetails(
            name="ghidra-psx-ldr", description="PSX loader", author="lab313ru"))
    launcher.start()

    from ghidra.program.model.data import ShortDataType
    from ghidra.util.task import ConsoleTaskMonitor
    monitor = ConsoleTaskMonitor()

    with pyghidra.open_program(
        binary_path=None, project_location=PROJECT, project_name=NAME,
        program_name=PROGRAM, analyze=False, nested_project_location=False,
    ) as flat_api:
        program = flat_api.getCurrentProgram()
        dtm = program.getDataTypeManager()

        def comp_at(struct, off):
            for c in struct.getDefinedComponents():
                if c.getOffset() == off:
                    return c
            return None

        txid = program.startTransaction("audit: struct naming/congruence fixes")
        changed = 0
        try:
            # #1 retype
            for path, off, name, cmt in RETYPE_SHORT:
                st = dtm.getDataType(path)
                if st is None:
                    log(f"  !! missing {path}"); continue
                c = comp_at(st, off)
                old = f"{c.getDataType().getName()} {c.getFieldName()}" if c else "(none)"
                st.replaceAtOffset(off, ShortDataType.dataType, 2, name, cmt)
                log(f"  RETYPE {path}+0x{off:x}: {old} -> short {name}")
                changed += 1

            # #2 renames
            for path, off, newname in RENAMES:
                st = dtm.getDataType(path)
                if st is None:
                    log(f"  !! missing {path}"); continue
                c = comp_at(st, off)
                if c is None:
                    log(f"  !! no component {path}+0x{off:x}"); continue
                oldname = c.getFieldName()
                if oldname == newname:
                    log(f"  skip  {path}+0x{off:x}: already {newname}"); continue
                c.setFieldName(newname)
                log(f"  RENAME {path}+0x{off:x}: {oldname} -> {newname}")
                changed += 1

            # #6 delete obsolete types
            for path in DELETE_TYPES:
                dt = dtm.getDataType(path)
                if dt is None:
                    log(f"  skip  delete {path}: not found"); continue
                if dtm.remove(dt, monitor):
                    log(f"  DELETE {path}"); changed += 1
                else:
                    log(f"  !! could not delete {path}")
        finally:
            program.endTransaction(txid, True)

        log(f"Applied {changed} change(s). Saving...")
        program.getDomainFile().save(monitor)
        log("Saved.")


if __name__ == "__main__":
    main()
