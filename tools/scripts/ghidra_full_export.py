#!/usr/bin/env python3
"""
Headless full export of the Skullmonkeys Ghidra program for offline auditing.

Produces three artifacts under the output dir (default /tmp):
  sm_types.txt   - every composite (struct/union) with field offset/size/type/
                   name/comment, every enum's members, and typedefs. The
                   backbone for struct-field naming-congruence checks.
  sm_funcs.txt   - every function: address, signature, namespace, plate comment.
  sm_export.c    - best-effort whole-program C export via Ghidra's CppExporter
                   (decompiled bodies + datatype defs). Skipped gracefully if
                   the exporter API differs on this Ghidra build.

Run inside `nix develop` so pyghidra + GHIDRA_INSTALL_DIR are available, e.g.:
  nix develop -c python3 tools/scripts/ghidra_full_export.py \
      --project /mnt/share/sam/ghidra --name skullmonkeys --program SLES_010.90
"""
import argparse
import sys
from pathlib import Path

PSX_EXT_CANDIDATES = [
    "z088mpgaxbk78rvr4c011pbk2k50ag2p-ghidra-psx-ldr-2025.09.06/lib/ghidra/Ghidra/Extensions/ghidra-psx-ldr",
]


def log(*a):
    print(*a, file=sys.stderr, flush=True)


def dump_types(program, out_path):
    """Reliable, hand-rolled dump of all composites/enums/typedefs."""
    from ghidra.program.model.data import Composite, Union, Enum, TypeDef
    dtm = program.getDataTypeManager()
    composites, enums, typedefs = [], [], []
    for dt in dtm.getAllDataTypes():
        if isinstance(dt, Composite):
            composites.append(dt)
        elif isinstance(dt, Enum):
            enums.append(dt)
        elif isinstance(dt, TypeDef):
            typedefs.append(dt)

    composites.sort(key=lambda d: d.getPathName())
    enums.sort(key=lambda d: d.getPathName())
    typedefs.sort(key=lambda d: d.getPathName())

    lines = []
    lines.append(f"# composites={len(composites)} enums={len(enums)} typedefs={len(typedefs)}\n")
    for c in composites:
        kind = "union" if isinstance(c, Union) else "struct"
        try:
            sz = c.getLength()
        except Exception:
            sz = "?"
        lines.append(f"\n{kind} {c.getPathName()} (size=0x{sz:x})" if isinstance(sz, int)
                     else f"\n{kind} {c.getPathName()} (size={sz})")
        for comp in c.getComponents():
            off = comp.getOffset()
            length = comp.getLength()
            fname = comp.getFieldName() or "(unnamed)"
            ftype = comp.getDataType().getName()
            cmt = comp.getComment() or ""
            cmt = (" // " + cmt.replace("\n", " ")) if cmt else ""
            lines.append(f"  +0x{off:03x} [0x{length:x}] {ftype:28} {fname}{cmt}")
    for e in enums:
        try:
            sz = e.getLength()
        except Exception:
            sz = "?"
        lines.append(f"\nenum {e.getPathName()} (size={sz})")
        for nm in e.getNames():
            lines.append(f"  {nm} = {e.getValue(nm)}")
    lines.append("\n# typedefs")
    for t in typedefs:
        try:
            base = t.getDataType().getName()
        except Exception:
            base = "?"
        lines.append(f"typedef {base} {t.getPathName()}")

    Path(out_path).write_text("\n".join(lines) + "\n")
    log(f"  wrote {out_path}: {len(composites)} composites, {len(enums)} enums, {len(typedefs)} typedefs")


def dump_functions(program, out_path):
    """Address, signature, namespace, plate comment for every function."""
    fm = program.getFunctionManager()
    listing = program.getListing()
    funcs = list(fm.getFunctions(True))
    funcs.sort(key=lambda f: f.getEntryPoint().getOffset())
    lines = [f"# functions={len(funcs)}\n"]
    for f in funcs:
        ep = f.getEntryPoint()
        sig = f.getSignature().getPrototypeString()
        ns = f.getParentNamespace().getName(True) if f.getParentNamespace() else ""
        plate_cmt = ""
        cu = listing.getCodeUnitAt(ep)
        if cu is not None:
            try:
                from ghidra.program.model.listing import CodeUnit
                pc = cu.getComment(CodeUnit.PLATE_COMMENT)
            except Exception:
                try:
                    from ghidra.program.model.listing import CommentType
                    pc = cu.getComment(CommentType.PLATE)
                except Exception:
                    pc = None
            plate_cmt = (pc or "").replace("\n", " \\n ")
        ns_s = f" ns={ns}" if ns and ns != "Global" else ""
        lines.append(f"{ep}  {sig}{ns_s}")
        if plate_cmt:
            lines.append(f"    ; {plate_cmt}")
    Path(out_path).write_text("\n".join(lines) + "\n")
    log(f"  wrote {out_path}: {len(funcs)} functions")


def export_c(program, out_path):
    """Best-effort whole-program C export via CppExporter."""
    try:
        from ghidra.app.util.exporter import CppExporter
        from ghidra.util.task import ConsoleTaskMonitor
        from java.io import File as JFile
        from jpype import JImplements, JOverride
        from ghidra.app.util import DomainObjectService

        @JImplements(DomainObjectService)
        class _DOS:
            def __init__(self, p):
                self._p = p

            @JOverride
            def getDomainObject(self):
                return self._p

        exporter = CppExporter()
        try:
            options = exporter.getOptions(_DOS(program))
            for opt in options:
                name = opt.getName()
                if "Emit Data-type" in name or name == "Create C File":
                    opt.setValue(True)
                elif name == "Create Header File":
                    opt.setValue(False)
                elif "C++ Style" in name:
                    opt.setValue(True)
            exporter.setOptions(options)
        except Exception as e:
            log(f"  CppExporter option setup failed (using defaults): {e}")
        ok = exporter.export(JFile(out_path), program, None, ConsoleTaskMonitor())
        log(f"  CppExporter export -> {out_path} (ok={ok})")
    except Exception as e:
        log(f"  CppExporter unavailable/failed, skipping sm_export.c: {e}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--project", default="/mnt/share/sam/ghidra")
    ap.add_argument("--name", default="skullmonkeys")
    ap.add_argument("--program", default="SLES_010.90")
    ap.add_argument("--out", default="/tmp")
    ap.add_argument("--no-c", action="store_true", help="skip the slow CppExporter pass")
    args = ap.parse_args()

    import pyghidra
    from pyghidra import HeadlessPyGhidraLauncher, ExtensionDetails

    launcher = HeadlessPyGhidraLauncher(verbose=True)
    for rel in PSX_EXT_CANDIDATES:
        p = Path("/nix/store") / rel
        if p.exists():
            log(f"Installing PSX extension: {p}")
            launcher.install_plugin(p, ExtensionDetails(
                name="ghidra-psx-ldr",
                description="Sony PlayStation PSX executables loader for Ghidra",
                author="lab313ru"))
            break
    launcher.start()

    log(f"Opening {args.project}/{args.name} :: {args.program}")
    with pyghidra.open_program(
        binary_path=None,
        project_location=args.project,
        project_name=args.name,
        program_name=args.program,
        analyze=False,
        nested_project_location=False,
    ) as flat_api:
        program = flat_api.getCurrentProgram()
        log(f"Opened: {program.getName()}")
        out = Path(args.out)
        dump_types(program, out / "sm_types.txt")
        dump_functions(program, out / "sm_funcs.txt")
        if not args.no_c:
            export_c(program, str(out / "sm_export.c"))
    log("Done.")


if __name__ == "__main__":
    main()
