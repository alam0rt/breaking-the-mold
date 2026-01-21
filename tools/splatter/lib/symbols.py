"""Symbol address file parsing."""

import re
from pathlib import Path
from dataclasses import dataclass
from typing import Optional


@dataclass
class Symbol:
    """A symbol from symbol_addrs.txt."""
    name: str
    address: int
    size: Optional[int] = None
    type: Optional[str] = None
    
    @property
    def end_address(self) -> Optional[int]:
        """Calculate end address if size is known."""
        if self.size is not None:
            return self.address + self.size
        return None


def parse_symbol_addrs(path: Path) -> dict:
    """
    Parse symbol_addrs.txt and return a dict of name -> Symbol.
    
    Format:
        FunctionName = 0x80012345; // size:0x88 type:func
    """
    symbols = {}
    
    # Pattern: name = 0xADDRESS; // optional annotations
    pattern = re.compile(
        r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*0x([0-9a-fA-F]+)\s*;'
        r'(?:\s*//\s*(.*))?$'
    )
    
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            
            match = pattern.match(line)
            if not match:
                continue
            
            name = match.group(1)
            address = int(match.group(2), 16)
            annotations = match.group(3) or ''
            
            # Parse annotations
            size = None
            sym_type = None
            
            size_match = re.search(r'size:0x([0-9a-fA-F]+)', annotations)
            if size_match:
                size = int(size_match.group(1), 16)
            
            type_match = re.search(r'type:(\w+)', annotations)
            if type_match:
                sym_type = type_match.group(1)
            
            symbols[name] = Symbol(
                name=name,
                address=address,
                size=size,
                type=sym_type
            )
    
    return symbols


def find_symbol(symbols: dict, name: str) -> Optional[Symbol]:
    """Find a symbol by name (case-sensitive)."""
    return symbols.get(name)
