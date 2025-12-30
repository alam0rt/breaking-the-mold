# Disks Directory

Place your original PSX game binaries in this directory.

## Structure

```
disks/
├── us/           # US version
│   └── SLUS_XXX.XX
├── eu/           # European version  
│   └── SLES_XXX.XX
└── jp/           # Japanese version
    └── SLPS_XXX.XX
```

## Extracting from CD Images

If you have a BIN/CUE CD image, you can extract files using:

```bash
# Convert BIN/CUE to ISO
bchunk game.bin game.cue game

# Mount the ISO (Linux)
sudo mount -o loop game01.iso /mnt/cdrom

# Copy the executable
cp /mnt/cdrom/SLUS_XXX.XX disks/us/
```

## Important

- **Do not commit game binaries** - they are copyrighted material
- You must own a legal copy of the game
- The `.gitignore` excludes files in this directory
