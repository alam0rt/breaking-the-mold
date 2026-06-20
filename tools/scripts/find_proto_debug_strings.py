#!/usr/bin/env python3
"""Locate prototype-debug format strings inside the prototype PSX EXE."""
import sys

with open('disks/prototype/ext/SLUS_006.01', 'rb') as f:
    data = f.read()

strings = [b'MISSING SEQ', b'Layers = ', b'AL# = ', b'TUs = ', b'ram u=',
           b'FC8B=', b'ProgEnd', b'%d:(%d,%d)']
for s in strings:
    idx = data.find(s)
    if idx < 0:
        print(f'{s.decode():15s} NOT FOUND')
        continue
    vaddr = idx - 0x800 + 0x80010000
    end = data.find(b'\x00', idx)
    snippet = data[idx:end].decode(errors='replace')
    print(f'{s.decode():15s} vaddr={vaddr:#x} off={idx:#x}  -> {snippet!r}')
