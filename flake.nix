# Nix Flake for PSX Decompilation
# ================================
# This flake provides a complete development environment for PSX decompilation.
#
# Uses pre-built PSX compiler tools (cc1-psx-26) which are downloaded separately.
# Run ./tools/download-toolchain.sh to get the compiler.

{
  description = "My PSX Game Decompilation Project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixgl.url = "github:nix-community/nixGL";
    pcsx-redux.url = "github:grumpycoders/pcsx-redux";
    #pcsx-redux.inputs.nixpkgs.follows = "nixpkgs-unstable";
  };

  outputs = { self, nixpkgs, flake-utils, nixgl, pcsx-redux }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          overlays = [ nixgl.overlay ];
        };

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
          coverage
          pip
          virtualenv
        ]);

        # Cross-compilation toolchain for MIPS (PSX uses MIPS R3000)
        mipsCross = pkgs.pkgsCross.mipsel-linux-gnu;

        # Wrap pcsx-redux with nixGLIntel for OpenGL support
        pcsx-redux-wrapped = pkgs.writeShellScriptBin "pcsx-redux" ''
          exec ${pkgs.nixgl.nixGLIntel}/bin/nixGLIntel ${pcsx-redux.packages.${system}.default}/bin/pcsx-redux "$@"
        '';

        # GDB wrapper for Ghidra debugger with required Python paths
        ghidra-gdb = pkgs.writeShellScriptBin "ghidra-gdb" ''
          export PYTHONPATH="${pkgs.ghidra}/lib/ghidra/Ghidra/Debug/Debugger-agent-gdb/pypkg/src:${pkgs.ghidra}/lib/ghidra/Ghidra/Debug/Debugger-rmi-trace/pypkg/src''${PYTHONPATH:+:$PYTHONPATH}"
          exec ${pkgs.gdb}/bin/gdb "$@"
        '';

      in
      {
        devShells.default = pkgs.mkShell {
          name = "psx-decomp";

          buildInputs = with pkgs; [
            # Core build tools
            gnumake
            ninja
            cmake
            pkg-config

            # MIPS cross-compilation toolchain
            pkgsCross.mipsel-linux-gnu.buildPackages.binutils
            pkgsCross.mipsel-linux-gnu.buildPackages.gcc

            # Python
            pythonEnv
            uv

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

            # Debugging
            gdb
            ghidra-gdb

            # openGL on nix
            pkgs.nixgl.nixGLIntel

            # PSX emulator for testing (wrapped with nixGLIntel)
            pcsx-redux-wrapped
          ];

          shellHook = ''
            echo "🎮 PSX Decompilation Environment"
            echo "================================"
            echo ""

            # Set library path for Python packages that need libstdc++
            export LD_LIBRARY_PATH="${pkgs.stdenv.cc.cc.lib}/lib:$LD_LIBRARY_PATH"

            # Alias for pcsx-redux with impure nixGL (for non-NixOS systems)
            # Use this if the wrapped version doesn't work with your GPU drivers
            alias pcsx-redux-impure='nix run --impure github:nix-community/nixGL#nixGLIntel -- ${pcsx-redux.packages.${system}.default}/bin/pcsx-redux'

            # Python virtual environment
            if [ ! -d .venv ]; then
              python -m venv .venv
            fi
            source .venv/bin/activate

            # Install Python requirements
            if [ -f tools/requirements-python.txt ]; then
              pip install -q -r tools/requirements-python.txt
            fi

            # Ensure bin directory exists for downloaded tools
            mkdir -p bin

            # Download PSX compiler tools if not present
            if [ ! -f bin/cc1-psx-26 ]; then
              echo ""
              echo "⚠️  PSX compiler not found. Run: ./tools/download-toolchain.sh"
              echo ""
            fi

            # Add tools to PATH
            export PATH="$PWD/bin:$PWD/tools:$PATH"

            # Source convenient aliases
            if [ -f tools/.bash_aliases ]; then
              source tools/.bash_aliases
            fi

            # Go workspace support
            if [ -f go.work ]; then
              export GOWORK="$PWD/go.work"
            fi

            echo "Toolchain:"
            echo "  MIPS binutils: $(mipsel-unknown-linux-gnu-as --version | head -1)"
            echo ""
            echo "Ready! Run 'make help' for available commands."
            echo ""
          '';

          LANG = "en_US.UTF-8";
        };
      }
    );
}
