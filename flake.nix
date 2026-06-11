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
    
    # PSX loader for Ghidra (from lab313ru) - pinned to last Ghidra 11.x compatible version
    ghidra-psx-ldr-src = {
      url = "github:lab313ru/ghidra_psx_ldr/2025.09.06";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, nixgl, pcsx-redux, ghidra-psx-ldr-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          overlays = [ nixgl.overlay ];
        };

        # Ghidra with PSX loader extension
        ghidra-psx-ldr = pkgs.ghidra.buildGhidraExtension {
          pname = "ghidra-psx-ldr";
          version = "2025.09.06";
          src = ghidra-psx-ldr-src;
          preBuild = ''
            rm -f data/languages/mips32le.sla
            ${pkgs.ghidra}/lib/ghidra/support/sleigh data/languages/mips32le.slaspec data/languages/mips32le.sla
          '';
          meta.description = "Sony PlayStation PSX executables loader for Ghidra";
        };
        ghidraWithPsx = pkgs.ghidra.withExtensions (exts: [ ghidra-psx-ldr ]);

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
          jpype1      # Required for pyghidra
        ]);

        # PyGhidra environment with Ghidra path configured
        pyghidraEnv = pkgs.python3.withPackages (ps: with ps; [
          jpype1
          pip
        ]);

        # Cross-compilation toolchain for MIPS (PSX uses MIPS R3000)
        mipsCross = pkgs.pkgsCross.mipsel-linux-gnu;

        # Wrap pcsx-redux with nixGLIntel for OpenGL support
        pcsx-redux-wrapped = pkgs.writeShellScriptBin "pcsx-redux" ''
          exec ${pkgs.nixgl.nixGLIntel}/bin/nixGLIntel ${pcsx-redux.packages.${system}.default}/bin/pcsx-redux "$@"
        '';

        # GDB wrapper for Ghidra debugger with required Python paths
        ghidra-gdb = pkgs.writeShellScriptBin "ghidra-gdb" ''
          export PYTHONPATH="${ghidraWithPsx}/lib/ghidra/Ghidra/Debug/Debugger-agent-gdb/pypkg/src:${ghidraWithPsx}/lib/ghidra/Ghidra/Debug/Debugger-rmi-trace/pypkg/src''${PYTHONPATH:+:$PYTHONPATH}"
          exec ${pkgs.gdb}/bin/gdb "$@"
        '';

        # RAM address <-> File offset converters
        # RAM base: 0x80010000, File header: 0x800
        ram2off = pkgs.writeShellScriptBin "ram2off" ''
          if [ -z "$1" ]; then
            echo "Usage: ram2off <RAM_ADDR>"
            echo "Convert PSX RAM address to file offset"
            echo "Example: ram2off 80010068"
            exit 1
          fi
          ${pkgs.bc}/bin/bc <<< "obase=16; ibase=16; ''${1^^} - 80010000 + 800"
        '';

        off2ram = pkgs.writeShellScriptBin "off2ram" ''
          if [ -z "$1" ]; then
            echo "Usage: off2ram <FILE_OFFSET>"
            echo "Convert file offset to PSX RAM address"
            echo "Example: off2ram 39F0"
            exit 1
          fi
          ${pkgs.bc}/bin/bc <<< "obase=16; ibase=16; ''${1^^} + 80010000 - 800"
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

            # Java (required for PyGhidra)
            temurin-bin-21

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
            luaPackages.luacheck  # Lua linter

            # Debugging
            gdb
            ghidra-gdb

            # Address conversion utilities
            ram2off
            off2ram

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

            # Install PyGhidra for headless Ghidra API access
            # (installed in venv to avoid Nix packaging complexity)
            pip install -q pyghidra

            # Set Ghidra install directory (required for PyGhidra)
            # Point to base Ghidra, extensions are loaded from user config
            export GHIDRA_INSTALL_DIR="${pkgs.ghidra}/lib/ghidra"

            # Allow the Ghidra MCP plugin to run inline scripts (run_script_inline).
            # The gate is checked inside the Ghidra process; loopback-only, no auth token.
            export GHIDRA_MCP_ALLOW_SCRIPTS=1

            # Install PSX loader extension to user's Ghidra config
            # Must COPY (not symlink) due to Ghidra's case-sensitive path validation
            GHIDRA_VERSION=$(${pkgs.ghidra}/lib/ghidra/support/launch.sh --version 2>/dev/null | grep -oP '\d+\.\d+\.\d+' | head -1 || echo "11.4.2")
            GHIDRA_EXT_DIR="$HOME/.config/ghidra/ghidra_''${GHIDRA_VERSION}_NIX/Extensions"
            PSX_EXT_SRC="${ghidra-psx-ldr}/lib/ghidra/Ghidra/Extensions/ghidra-psx-ldr"
            if [ ! -d "$GHIDRA_EXT_DIR/ghidra-psx-ldr" ] || [ -L "$GHIDRA_EXT_DIR/ghidra-psx-ldr" ]; then
              mkdir -p "$GHIDRA_EXT_DIR"
              rm -rf "$GHIDRA_EXT_DIR/ghidra-psx-ldr"  # Remove old symlink if exists
              cp -r "$PSX_EXT_SRC" "$GHIDRA_EXT_DIR/"
              chmod -R u+w "$GHIDRA_EXT_DIR/ghidra-psx-ldr"
              echo "  Installed PSX loader extension to $GHIDRA_EXT_DIR"
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
            if [ -n "$GHIDRA_INSTALL_DIR" ]; then
              echo "  PyGhidra: $GHIDRA_INSTALL_DIR"
            else
              echo "  PyGhidra: ⚠️  Set GHIDRA_INSTALL_DIR to enable"
            fi
            echo ""
            echo "Ready! Run 'make help' for available commands."
            echo ""
          '';

          LANG = "en_US.UTF-8";
        };
      }
    );
}
