#!/usr/bin/env bash
# One-time wineprefix setup for SN32 cc1.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
export WINEARCH=win32
export WINEPREFIX="$HERE/wineprefix"
export WINEDEBUG=-all

# Locate a wine binary (must already be cached/available; use nix shell otherwise).
WINE_BIN="${WINE_BIN:-}"
if [[ -z "$WINE_BIN" ]]; then
  for cand in /nix/store/*-wine-11.0/bin/wine; do
    [[ -x "$cand" ]] && WINE_BIN="$cand" && break
  done
fi
[[ -z "$WINE_BIN" ]] && { echo "wine not found; run inside 'nix shell nixpkgs#wine'"; exit 1; }

# Boot the prefix if needed.
if [[ ! -d "$WINEPREFIX/drive_c" ]]; then
  "$WINE_BIN" wineboot.exe --init >/dev/null 2>&1 || true
fi

mkdir -p "$WINEPREFIX/drive_c/tmp"

# cc1psx tmpnam returns paths like '/ctaNNNNN'; we make %TMP% point at C:\tmp
# so the absolute path becomes 'C:\tmp/ctaNNNNN' which wine can write to.
"$WINE_BIN" reg add 'HKCU\Environment' /v TMP    /t REG_SZ /d 'C:\tmp' /f >/dev/null 2>&1
"$WINE_BIN" reg add 'HKCU\Environment' /v TEMP   /t REG_SZ /d 'C:\tmp' /f >/dev/null 2>&1
"$WINE_BIN" reg add 'HKCU\Environment' /v TMPDIR /t REG_SZ /d 'C:\tmp' /f >/dev/null 2>&1

echo "OK: SN32 cc1 wineprefix ready at $WINEPREFIX"
