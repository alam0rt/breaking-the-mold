#!/usr/bin/env bash
# Run decomp-permuter on a remote host (default: sauron) using its CPUs.
#
# Usage:
#   tools/permuter-remote.sh FUNC_NAME [extra permuter args ...]
#
# Env vars:
#   REMOTE_HOST   ssh target            (default: sauron)
#   REMOTE_DIR    remote work dir       (default: ~/btm-permuter)
#   JOBS          -j value for permuter (default: 48 — leaves headroom on a 64-core box)
#   FULL_SYNC=1   re-sync everything (toolchain + sources). Otherwise only the
#                 nonmatchings/<FUNC>/ dir and source/include trees are re-sent.
#
# First-time setup is automatic; later runs only resync what changed.
set -euo pipefail

REMOTE_HOST="${REMOTE_HOST:-sauron}"
REMOTE_DIR="${REMOTE_DIR:-btm-permuter}"   # relative to remote $HOME
JOBS="${JOBS:-48}"
FULL_SYNC="${FULL_SYNC:-0}"

if [[ $# -lt 1 ]]; then
    echo "usage: $0 FUNC_NAME [extra permuter args ...]" >&2
    exit 2
fi

FUNC="$1"; shift
EXTRA_ARGS=("$@")

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

if [[ ! -d "nonmatchings/$FUNC" ]]; then
    echo "error: nonmatchings/$FUNC does not exist." >&2
    echo "       run: make permuter-import FILE=src/foo.c FUNC=$FUNC" >&2
    exit 1
fi

echo "==> remote = $REMOTE_HOST:~/$REMOTE_DIR    func = $FUNC    -j$JOBS"

# 1. Ensure the remote dir exists.
ssh "$REMOTE_HOST" "mkdir -p '$REMOTE_DIR/nonmatchings'"

RSYNC_COMMON=(rsync -a --delete --info=stats0 -e ssh
              --exclude='__pycache__' --exclude='*.pyc'
              --exclude='.git' --exclude='output-*')

# 2. Toolchain / permuter source. Heavy parts (cc1, decomp-permuter, maspsx)
# are skipped after first bootstrap. Wrapper scripts are always resynced.
# Preserve the `tools/` and `bin/` path prefixes — the permuter wrappers and
# nonmatchings/*/compile.sh reference them as relative paths.
if [[ "$FULL_SYNC" == "1" ]] || ! ssh "$REMOTE_HOST" "test -f '$REMOTE_DIR/.bootstrapped'"; then
    echo "==> bootstrapping toolchain + permuter sources"
    ssh "$REMOTE_HOST" "mkdir -p '$REMOTE_DIR/bin' '$REMOTE_DIR/tools'"
    "${RSYNC_COMMON[@]}" bin/cc1-psx-26 "$REMOTE_HOST":"$REMOTE_DIR/bin/"
    "${RSYNC_COMMON[@]}" \
        tools/gcc-2.7.2-psx \
        tools/maspsx \
        tools/decomp-permuter \
        "$REMOTE_HOST":"$REMOTE_DIR/tools/"
    ssh "$REMOTE_HOST" "touch '$REMOTE_DIR/.bootstrapped'"
fi

# Wrapper scripts: always resync (cheap, and we iterate on them).
echo "==> syncing wrapper scripts"
"${RSYNC_COMMON[@]}" \
    tools/permuter_cc1_maspsx.sh \
    tools/permuter_as.sh \
    tools/permuter_settings_psx.toml \
    tools/permuter-remote-run.sh \
    "$REMOTE_HOST":"$REMOTE_DIR/tools/"
ssh "$REMOTE_HOST" "chmod +x '$REMOTE_DIR/tools/permuter-remote-run.sh' \
                              '$REMOTE_DIR/tools/permuter_cc1_maspsx.sh' \
                              '$REMOTE_DIR/tools/permuter_as.sh'"

# 3. Headers + project sources (cheap, always sync).
# Note: include/ is required; psyq/ is referenced by the permuter wrappers as
# `-I psyq` but is harmless if missing (imported base.c rarely needs it).
echo "==> syncing include/ src/ (+ psyq/ if present)"
"${RSYNC_COMMON[@]}" include src "$REMOTE_HOST":"$REMOTE_DIR/"
if [[ -d psyq ]]; then
    "${RSYNC_COMMON[@]}" psyq "$REMOTE_HOST":"$REMOTE_DIR/"
elif [[ -d disks/psyq ]]; then
    "${RSYNC_COMMON[@]}" disks/psyq/ "$REMOTE_HOST":"$REMOTE_DIR/psyq/"
fi

# 4. The specific nonmatchings/<FUNC>/ directory.
echo "==> syncing nonmatchings/$FUNC/"
"${RSYNC_COMMON[@]}" "nonmatchings/$FUNC/" \
    "$REMOTE_HOST":"$REMOTE_DIR/nonmatchings/$FUNC/"

# 5. Kick off the remote run.
ssh -t "$REMOTE_HOST" \
    "cd '$REMOTE_DIR' && ./tools/permuter-remote-run.sh '$FUNC' -j '$JOBS' ${EXTRA_ARGS[*]:-}"
