#!/bin/bash
# =============================================================================
# PSX Toolchain Download Script
# =============================================================================
# Downloads the old-gcc compiler toolchain for PSX decompilation.
#
# This script downloads prebuilt binaries from decompals/old-gcc which are
# native Linux executables that match the original PSY-Q compiler output.
#
# No wine/dosemu2 required! This is the MODERN approach to PSX decomp.
#
# For more info see:
#   - https://github.com/decompals/old-gcc
#   - https://github.com/mkst/maspsx
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "==================================="
echo "PSX Toolchain Download (old-gcc)"
echo "==================================="
echo ""

# -----------------------------------------------------------------------------
# Configuration
# -----------------------------------------------------------------------------

# Default GCC version - GCC 2.95.2 matches PSY-Q 4.6 (most common)
# Change this to match your game's compiler version:
#   2.95.2    - PSY-Q 4.6 (CC1PSX.EXE)
#   2.91.66   - PSY-Q 4.4/4.5 (egcs 1.1.2 based)
#   2.8.1     - PSY-Q 4.0-4.3
#   2.8.0     - Earlier PSY-Q 4.x
#   2.7.2     - PSY-Q 3.x
#   2.6.3     - Early PSY-Q
#   2.6.0     - Very early PSY-Q
#
# PSX-specific versions (with -msoft-float, -msplit-addresses, -mgpopt):
#   2.95.2-psx, 2.91.66-psx, 2.8.1-psx, 2.8.0-psx, 2.7.2-psx, etc.

GCC_VERSION="${1:-2.95.2}"
GCC_DIR="$PROJECT_DIR/tools/gcc-$GCC_VERSION"
OLD_GCC_RELEASES="https://github.com/decompals/old-gcc/releases/download/release"

mkdir -p "$GCC_DIR"

echo "Downloading GCC $GCC_VERSION from decompals/old-gcc..."
echo ""

# -----------------------------------------------------------------------------
# Download as tarball (most reliable)
# -----------------------------------------------------------------------------

TARBALL_URL="$OLD_GCC_RELEASES/gcc-$GCC_VERSION.tar.gz"
TARBALL_PATH="/tmp/gcc-$GCC_VERSION.tar.gz"

if [ -f "$GCC_DIR/cc1" ]; then
    echo "✓ GCC $GCC_VERSION already installed"
else
    echo "Downloading gcc-$GCC_VERSION.tar.gz..."
    
    if curl -fL "$TARBALL_URL" -o "$TARBALL_PATH"; then
        echo "Extracting..."
        tar -xzf "$TARBALL_PATH" -C "$GCC_DIR" 2>/dev/null || true
        
        # Make executables
        chmod +x "$GCC_DIR"/* 2>/dev/null || true
        
        echo "✓ Downloaded and extracted gcc-$GCC_VERSION"
        rm -f "$TARBALL_PATH"
    else
        echo "✗ Could not download tarball"
        echo ""
        echo "Available versions at: https://github.com/decompals/old-gcc/releases"
        echo ""
        echo "You can also build from source:"
        echo "  git clone https://github.com/decompals/old-gcc"
        echo "  cd old-gcc && make VERSION=$GCC_VERSION"
        exit 1
    fi
fi

# -----------------------------------------------------------------------------
# Verify installation
# -----------------------------------------------------------------------------

echo ""
echo "Verifying installation..."

if [ -f "$GCC_DIR/cc1" ]; then
    VERSION_OUTPUT=$("$GCC_DIR/cc1" --version 2>&1 | head -1 || echo "")
    if [ -n "$VERSION_OUTPUT" ]; then
        echo "✓ cc1: $VERSION_OUTPUT"
    else
        echo "✓ cc1 exists"
    fi
else
    echo "✗ cc1 not found - download may have failed"
    exit 1
fi

# -----------------------------------------------------------------------------
# Create convenience wrapper script
# -----------------------------------------------------------------------------

# Create an 'as' wrapper that invokes maspsx
AS_WRAPPER="$GCC_DIR/as"
if [ ! -f "$AS_WRAPPER" ]; then
    cat > "$AS_WRAPPER" << 'EOF'
#!/bin/bash
# maspsx wrapper - replicates ASPSX.EXE behavior for GNU as
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 "$SCRIPT_DIR/../maspsx/maspsx.py" \
    --run-assembler \
    --aspsx-version=2.86 \
    --macro-inc \
    --use-comm-section \
    "$@"
EOF
    chmod +x "$AS_WRAPPER"
    echo "✓ Created maspsx 'as' wrapper"
fi

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------

echo ""
echo "==================================="
echo "Toolchain setup complete!"
echo "==================================="
echo ""
echo "GCC $GCC_VERSION installed in: $GCC_DIR"
echo ""
echo "Files:"
ls -la "$GCC_DIR" 2>/dev/null | grep -v "^total" | head -10
echo ""
echo "To use a different GCC version, run:"
echo "  $0 <version>"
echo ""
echo "Common versions:"
echo "  2.95.2     - PSY-Q 4.6 (most common)"
echo "  2.91.66    - PSY-Q 4.4/4.5 (egcs 1.1.2)"
echo "  2.8.1      - PSY-Q 4.0-4.3"
echo "  2.7.2      - PSY-Q 3.x"
echo ""
echo "Next steps:"
echo "  1. Update GCC_VERSION in Makefile if needed"
echo "  2. Run: make detect-compiler  (analyze your binary)"
echo "  3. Run: make extract"
echo "  4. Run: make"
echo ""
