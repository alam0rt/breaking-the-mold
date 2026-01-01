# Ghidra Scripts for BTM Decompilation

## Scripts

### `GenerateSplatConfig.py` (Jython - Standard Ghidra)

Generates a splat-compatible YAML segment configuration from a Ghidra project.
**Use this for standard Ghidra installations.**

### `generate_splat_config_pyghidra.py` (Python 3 - PyGhidra only)

Same functionality but for PyGhidra installations only.

## Usage

### Method 1: Ghidra Script Manager (GUI) - RECOMMENDED

1. Open your project in Ghidra
2. Go to `Window` → `Script Manager`
3. Click the "Manage Script Directories" button (folder icon with green +)
4. Click "Add" and select: `tools/ghidra_scripts`
5. In the filter box, type "Splat"
6. Find `GenerateSplatConfig.py` in the script list
7. Double-click to run (or right-click → Run)
8. Output appears in the **Console** window (`Window` → `Console`)

### Method 2: Tools Menu

After adding the script directory, find it under:
`Tools` → `Generate Splat Config`

### Method 3: Headless Ghidra

```bash
$GHIDRA_HOME/support/analyzeHeadless /path/to/project ProjectName \
    -import disks/pal/SLES_010.90 \
    -postScript GenerateSplatConfig.py \
    -scriptPath tools/ghidra_scripts
```

### Method 4: PyGhidra (Python 3 only)

```bash
pip install pyghidra
python3 -m pyghidra.run_script \
    disks/pal/SLES_010.90 \
    tools/ghidra_scripts/generate_splat_config_pyghidra.py
```

## Output

The script outputs YAML configuration to the console:

```yaml
name: SLES_010_90
options:
  basename: game
  platform: psx
  compiler: PSYQ
  gp_value: 0x800A5954
  ...

segments:
  - name: header
    type: header
    start: 0x0

  - name: main
    type: code
    start: 0x800
    vram: 0x80010000
    bss_size: 0x82C0
    subsegments:
      # .rodata: 0x80010000 - 0x800131EF
      - [0x800, rodata]
      # .text: 0x800131F0 - 0x800907EB
      - [0x39F0, asm]
      ...
```

## Notes

- The script auto-detects section types based on memory block properties
- BSS size is calculated from uninitialized memory blocks
- GP value is found from `_gp` or similar symbols if present
- You may need to adjust file offsets based on your specific binary format
- PSX-EXE files typically have a 0x800 byte header

## Troubleshooting

### "No sections found"
Make sure the binary is properly analyzed. Run auto-analysis first:
`Analysis` → `Auto Analyze...`

### "Wrong file offsets"
The script assumes standard PSX-EXE format. For different loaders or formats,
you may need to adjust the `calculate_file_offset()` function.

### "Missing GP value"  
Add the `_gp` symbol manually in Ghidra, or set it in the splat config to
your `.sdata` start address + 0x7FF0.
