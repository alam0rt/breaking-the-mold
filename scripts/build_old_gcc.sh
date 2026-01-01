#!/usr/bin/env bash
# Download or build old GCC versions for PSX decompilation
# Usage: ./scripts/build_old_gcc.sh [version]
# Example: ./scripts/build_old_gcc.sh 2.7.2-psx
#          ./scripts/build_old_gcc.sh all-psx

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OLD_GCC_DIR="$PROJECT_ROOT/tools/old-gcc"
BUILD_DIR="$PROJECT_ROOT/tools/gcc-builds"

# GitHub releases URL
RELEASE_VERSION="0.13"
RELEASE_URL="https://github.com/decompals/old-gcc/releases/download/$RELEASE_VERSION"

# PSX-specific GCC versions (these have Sony PSX patches)
PSX_VERSIONS=(
    "2.5.7-psx"
    "2.6.0-psx"
    "2.6.3-psx"
    "2.7.2-psx"
    "2.8.0-psx"
    "2.8.1-psx"
    "2.91.66-psx"
    "2.95.2-psx"
)

# All available versions
ALL_VERSIONS=(
    "2.5.7" "2.5.7-psx"
    "2.6.0" "2.6.0-psx"
    "2.6.3" "2.6.3-psx"
    "2.7.0"
    "2.7.1"
    "2.7.2" "2.7.2-psx" "2.7.2-cdk" "2.7.2.1" "2.7.2.2" "2.7.2.3"
    "2.8.0" "2.8.0-psx"
    "2.8.1" "2.8.1-psx"
    "2.91.66" "2.91.66-psx"
    "2.95.2" "2.95.2-psx"
)

download_version() {
    local version="$1"
    local output_dir="$BUILD_DIR/gcc-$version"
    local tarball="gcc-$version.tar.gz"
    local url="$RELEASE_URL/$tarball"
    
    if [ -d "$output_dir" ] && [ -f "$output_dir/cc1" ]; then
        echo "[SKIP] GCC $version already exists at $output_dir"
        return 0
    fi
    
    echo "[DOWNLOAD] Fetching GCC $version from releases..."
    mkdir -p "$BUILD_DIR"
    
    # Download tarball
    if ! curl -L -o "$BUILD_DIR/$tarball" "$url" 2>/dev/null; then
        echo "[ERROR] Failed to download GCC $version"
        return 1
    fi
    
    # Extract
    mkdir -p "$output_dir"
    tar -xzf "$BUILD_DIR/$tarball" -C "$output_dir"
    rm "$BUILD_DIR/$tarball"
    
    # Verify cc1 exists
    if [ ! -f "$output_dir/cc1" ]; then
        echo "[ERROR] cc1 not found after extraction"
        rm -rf "$output_dir"
        return 1
    fi
    
    echo "[OK] GCC $version installed to $output_dir"
}

build_version() {
    local version="$1"
    local output_dir="$BUILD_DIR/gcc-$version"
    
    if [ -d "$output_dir" ] && [ -f "$output_dir/cc1" ]; then
        echo "[SKIP] GCC $version already built at $output_dir"
        return 0
    fi
    
    echo "[BUILD] Building GCC $version with Docker..."
    cd "$OLD_GCC_DIR"
    
    if ! VERSION="$version" make; then
        echo "[ERROR] Failed to build GCC $version"
        return 1
    fi
    
    # Move build output to our tools directory
    mkdir -p "$output_dir"
    mv "build-gcc-$version"/* "$output_dir/"
    rmdir "build-gcc-$version"
    
    echo "[OK] GCC $version built to $output_dir"
}

list_versions() {
    echo "Available GCC versions:"
    echo ""
    echo "PSX-specific (with Sony patches):"
    for v in "${PSX_VERSIONS[@]}"; do
        local status="[not installed]"
        if [ -f "$BUILD_DIR/gcc-$v/cc1" ]; then
            status="[installed]"
        fi
        echo "  - $v $status"
    done
    echo ""
    echo "Generic MIPS:"
    for v in "2.5.7" "2.6.0" "2.6.3" "2.7.0" "2.7.1" "2.7.2" "2.7.2.1" "2.7.2.2" "2.7.2.3" "2.7.2-cdk" "2.8.0" "2.8.1" "2.91.66" "2.95.2"; do
        local status="[not installed]"
        if [ -f "$BUILD_DIR/gcc-$v/cc1" ]; then
            status="[installed]"
        fi
        echo "  - $v $status"
    done
}

show_usage() {
    echo "Usage: $0 <version|all|all-psx|list|build-VERSION>"
    echo ""
    echo "Commands:"
    echo "  <version>       Download a prebuilt GCC version (e.g., 2.7.2-psx)"
    echo "  all-psx         Download all PSX-specific versions"
    echo "  all             Download all available versions"
    echo "  list            List available versions and install status"
    echo "  build-VERSION   Build from source with Docker (e.g., build-2.7.2-psx)"
    echo ""
    echo "Examples:"
    echo "  $0 2.7.2-psx        # Download GCC 2.7.2 with PSX patches"
    echo "  $0 all-psx          # Download all PSX versions"
    echo "  $0 list             # List available versions"
    echo "  $0 build-2.7.2-psx  # Build from source (requires Docker)"
}

# Main
case "${1:-}" in
    "")
        show_usage
        exit 1
        ;;
    "list")
        list_versions
        ;;
    "all-psx")
        for version in "${PSX_VERSIONS[@]}"; do
            download_version "$version"
        done
        echo ""
        echo "All PSX GCC versions downloaded!"
        ;;
    "all")
        for version in "${ALL_VERSIONS[@]}"; do
            download_version "$version"
        done
        echo ""
        echo "All GCC versions downloaded!"
        ;;
    build-*)
        version="${1#build-}"
        build_version "$version"
        ;;
    *)
        download_version "$1"
        ;;
esac
