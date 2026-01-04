#!/usr/bin/env python3
"""Check for unreferenced sectors in GAME.BLB"""
from blb import BLBFile, SECTOR_SIZE

with BLBFile('../disks/blb/GAME.BLB') as blb:
    file_size = blb._file.seek(0, 2)
    total_sectors = file_size // SECTOR_SIZE
    
    # Track all referenced sectors
    used_sectors = set()
    used_sectors.add(0)  # Header
    used_sectors.add(1)
    
    for i in range(blb.header.level_count):
        level = blb.header.get_level_by_index(i)
        
        # Primary
        for s in range(level.sector_offset, level.sector_offset + level.sector_count):
            used_sectors.add(s)
        
        # Secondary BASE
        for s in range(level.secondary_offset, level.secondary_offset + level.secondary_count):
            used_sectors.add(s)
        
        # Secondary SUB-BLOCKS (these are separate)
        for j in range(5):  # 5 sub-blocks
            if j < len(level.sec_sub_offsets) and j < len(level.sec_sub_counts):
                offset = level.sec_sub_offsets[j]
                count = level.sec_sub_counts[j]
                if offset > 0 and count > 0:
                    for s in range(offset, offset + count):
                        used_sectors.add(s)
        
        # Tertiary sub-blocks
        for j in range(level.tert_block_count):
            offset = level.tert_sub_offsets[j]
            count = level.tert_sub_counts[j]
            if offset > 0 and count > 0:
                for s in range(offset, offset + count):
                    used_sectors.add(s)
    
    print(f'Total sectors in file: {total_sectors}')
    print(f'Referenced sectors:    {len(used_sectors)}')
    print(f'Unreferenced sectors:  {total_sectors - len(used_sectors)}')
    print(f'Unreferenced bytes:    {(total_sectors - len(used_sectors)) * SECTOR_SIZE:,}')
    print()
    
    # Find contiguous unreferenced ranges
    unreferenced = sorted(set(range(total_sectors)) - used_sectors)
    
    # Group into ranges
    ranges = []
    if unreferenced:
        start = unreferenced[0]
        prev = start
        for s in unreferenced[1:]:
            if s != prev + 1:
                ranges.append((start, prev - start + 1))
                start = s
            prev = s
        ranges.append((start, prev - start + 1))
    
    print(f'Unreferenced ranges: {len(ranges)}')
    for sector, count in ranges[:20]:
        byte_off = sector * SECTOR_SIZE
        byte_size = count * SECTOR_SIZE
        print(f'  Sectors {sector}-{sector+count-1}: 0x{byte_off:X} ({byte_size:,} bytes)')
