#!/usr/bin/env python3
import struct

with open('/mnt/share/sam/code/btm/disks/blb/GAME.BLB', 'rb') as f:
    data = f.read(0x1000)

# Structure at 0xCD0 onwards (0x10 byte entries):
# +0x00: u16 unknown (possibly entry type/flags)
# +0x02: u8 unknown
# +0x03: 4-char level code (null-terminated)
# +0x08: 4 bytes (short name?)
# +0x0C: u16 sector offset
# +0x0E: u16 sector count

print('=== BLB Offset Table (0x10-byte entries starting at 0xCD0) ===')
print()

for i in range(20):
    offset = 0xCD0 + (i * 0x10)
    if offset >= 0x1000:
        break
    entry = data[offset:offset+16]

    # Parse the entry
    val0 = struct.unpack('<H', entry[0:2])[0]  # unknown
    val2 = entry[2]  # unknown byte
    code = entry[3:7].rstrip(b'\\x00').decode('ascii', errors='replace')
    short_name = entry[8:12].rstrip(b'\\x00').decode('ascii', errors='replace')
    sector_off = struct.unpack('<H', entry[12:14])[0]
    sector_cnt = struct.unpack('<H', entry[14:16])[0]

    byte_offset = sector_off * 2048
    byte_size = sector_cnt * 2048

    print(f'[{i:2d}] 0x{offset:04X}: code=\"{code:4s}\" short=\"{short_name:4s}\" sec_off=0x{sector_off:04X} sec_cnt=0x{sector_cnt:02X} -> BLB offset 0x{byte_offset:08X}, size 0x{byte_size:X}')
