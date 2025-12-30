# Nix Flake for PSX Decompilation
# ================================
# This flake provides a complete development environment for PSX decompilation.
# 
# Key feature: Uses GCC 2.95.3 from Nix's minimal-bootstrap, which matches
# the PSY-Q 4.6/4.7 compiler output. No external downloads required!

{
  description = "My PSX Game Decompilation Project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    
    # Pinned nixpkgs for minimal-bootstrap.gcc2 (GCC 2.95.3)
    # This provides a native Linux build of GCC 2.95.3 for PSX matching
    nixpkgs-gcc2.url = "github:NixOS/nixpkgs/9957cd48326fe8dbd52fdc50dd2502307f188b0d";
  };

  outputs = { self, nixpkgs, nixpkgs-gcc2, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
        
        # GCC 2.95.3 from minimal-bootstrap - matches PSY-Q 4.6/4.7
        pkgs-gcc2 = import nixpkgs-gcc2 {
          inherit system;
        };
        
        # The PSX-compatible GCC 2.95.3 compiler
        # This is a native x86_64 Linux build that cross-compiles to MIPS
        gcc295 = pkgs-gcc2.minimal-bootstrap.gcc2;

        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          pycparser
          pillow
          toml
          pyyaml
          intervaltree
          watchdog
          levenshtein
          cxxfilt
          tabulate
          requests
          graphviz
          black
          pip
          virtualenv
        ]);

      in
      {
        devShells.default = pkgs.mkShell {
          name = "psx-decomp";

          buildInputs = [
            # PSX Compiler - GCC 2.95.3 (matches PSY-Q 4.6/4.7)
            gcc295
          ] ++ (with pkgs; [
            # Core build tools
            gnumake
            ninja
            cmake
            pkg-config

            # MIPS cross-compilation (assembler, linker)
            pkgsCross.mipsel-linux-gnu.buildPackages.binutils

            # Python
            pythonEnv

            # Utilities
            git
            curl
            wget
            unzip
            p7zip
            clang-tools
            rustc
            cargo
            go
            bchunk
            libelf
            coreutils
            gawk
            gnused
            gnugrep
            findutils
            file
            which
            iconv
          ]);

          shellHook = ''
            echo "🎮 PSX Decompilation Environment (Modern Workflow)"
            echo "=================================================="
            echo ""
            echo "Toolchain:"
            echo "  GCC 2.95.3 (Nix minimal-bootstrap) - matches PSY-Q 4.6/4.7"
            echo "  MIPS binutils: $(mipsel-unknown-linux-gnu-as --version | head -1)"
            echo ""

            # Python virtual environment
            if [ ! -d .venv ]; then
              python -m venv .venv
            fi
            source .venv/bin/activate

            # Install Python requirements
            if [ -f tools/requirements-python.txt ]; then
              pip install -q -r tools/requirements-python.txt
            fi

            # Add tools to PATH
            export PATH="$PWD/tools:$PATH"

            # Source convenient aliases
            if [ -f tools/.bash_aliases ]; then
              source tools/.bash_aliases
            fi

            # Go workspace support
            if [ -f go.work ]; then
              export GOWORK="$PWD/go.work"
            fi

            echo "Ready! Run 'make help' for available commands."
            echo ""
          '';

          LANG = "en_US.UTF-8";
          
          # Export the GCC 2.95.3 path for Makefile
          GCC295_PATH = "${gcc295}";
        };
        
        # Export gcc295 for other flakes to use
        packages.gcc295 = gcc295;
      }
    );
}
