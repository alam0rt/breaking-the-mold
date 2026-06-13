#!/usr/bin/env bash
# Executed *on the remote host* by tools/permuter-remote.sh.
# Wraps the permuter in a one-shot ad-hoc `nix shell` providing python3+toml
# and the mipsel binutils that the compile/assemble wrappers expect.
#
# Usage (from $REMOTE_DIR): ./permuter-remote-run.sh FUNC [extra permuter args]
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: $0 FUNC_NAME [extra permuter args ...]" >&2
    exit 2
fi

FUNC="$1"; shift

# The vendored compile.sh hardcodes a path (from `permuter-import` on the
# original machine). Rewrite it to use this remote work dir + a fixed-up
# version of the wrapper that points at the bundled cc1.
WORK_DIR="$(pwd)"
COMPILE_SH="nonmatchings/$FUNC/compile.sh"
if [[ -f "$COMPILE_SH" ]]; then
    # Replace `cd <anything>` with `cd $WORK_DIR`.
    sed -i "s|^cd .*$|cd '$WORK_DIR'|" "$COMPILE_SH"
fi

# Tee output to /tmp/permuter-<func>.log on the remote so progress can be
# inspected even after the ssh session detaches (e.g. `tail -f`).
LOG="/tmp/permuter-${FUNC}.log"
echo "==> logging to ${REMOTE_HOST:-$(hostname)}:${LOG}"

# Pull `uv` and the mipsel binutils via a single ad-hoc nix-shell.
# `uv run` then handles python + the `toml` dep inline (no project venv).
exec nix-shell -p uv pkgsCross.mipsel-linux-gnu.buildPackages.binutils gcc --run \
    "uv run --quiet --with toml --python 3.12 \
        tools/decomp-permuter/permuter.py 'nonmatchings/$FUNC' $* \
        2>&1 | tee '$LOG'"
