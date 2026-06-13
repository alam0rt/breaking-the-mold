# gcc-2.7.2-sn32 — actual PSY-Q 4.0 cc1

The Win32 PE `CC1PSX.EXE` extracted from the Programmer Tools CD 2.0,
identified by its `strings` banner as:

    GNU C 2.7.2.SN32.3.7.0002 [AL 1.1, MM 40] Sony Playstation compiled by CC

(SHA-1 `728ce58aac328cbeba82a66348b71f9f79f2965a`, dated 14.5.97. The DOS/COFF
sibling `CC1PSX-DOS.EXE` is the corresponding DJGPP build of
`2.7.2.SN16.3.7.0002`, kept for reference but not wired up.)

## Usage

Wine is required. From inside `nix shell nixpkgs#wine` (one-time fetch),
the wrapper looks identical to every other `tools/gcc-*/cc1`:

```sh
./setup.sh                                   # one-time: wineprefix + TMP regs
./cc1 -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 -o out.s in.i
```

`./cc1` shells out to wine internally and behaves like a normal MIPS cc1 from
the caller's perspective.

## Why it's here

PSY-Q 4.0 is the only complete PSY-Q SDK we have on disk that ships a real
SN-Systems cc1 not in the decompals/old-gcc archive. It was tested
specifically as a candidate for breaking the **140-floor** on the
CLUT-effect tick-slot install in `func_80019F88` /
`InitCLUTColorLerpEffect`.

**Result:** byte-for-byte identical output to `tools/gcc-2.7.2-psx/cc1`
(decompals rebuild). See [docs/compiler-quirks.md Quirk 5](../../docs/compiler-quirks.md)
"Update 2026-06-13 — ruled out". The floor is **not** caused by a missing
patch level in the decompals rebuild.

## Compiler quirks

- Input must be preprocessed (`.i`), not `.c`. Same as every other PSY-Q
  cc1 — they don't ship a preprocessor.
- The compiler's `tmpnam()` returns a unix-style path; without `TMP`/`TEMP`
  set in the wineprefix registry it fails with `\/ctaNNNNN: No such file or
  directory`. `setup.sh` writes `HKCU\Environment\TMP=C:\tmp` once.
- Banner identifies itself with the SN build number `SN32.3.7.0002`. The
  PSY-Q 4.0 ASPSX bundled alongside it (`PSSN/BIN/ASPSX.EXE`) is version
  2.56 — older than the `--aspsx-version=2.86` we need for matching, so
  PSY-Q 4.0 is provably *not* the exact toolchain used for the original
  game. A later PSY-Q release (4.4+) with newer SN cc1 + ASPSX 2.86 would
  be the next candidate to try.
