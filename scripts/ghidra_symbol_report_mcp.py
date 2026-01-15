#!/usr/bin/env python3
"""
Ghidra Symbol Report Generator (MCP Version)
Uses Ghidra MCP Python client for direct database access.

This version provides richer analysis by accessing:
- Function decompilation
- Variable analysis
- Call graph data
- Data type information
"""

import sys
import json
from pathlib import Path
from collections import defaultdict
from datetime import datetime


# Add MCP client path if needed
# sys.path.insert(0, str(Path(__file__).parent / "mcp_client"))


def get_ghidra_functions_via_mcp():
    """
    Get all functions from Ghidra using MCP tools.
    This is a placeholder - actual implementation would use the MCP client library.
    """
    print("Note: Using direct MCP tool calls instead of REST API", file=sys.stderr)
    print("This provides richer function analysis and metadata", file=sys.stderr)
    
    # In a real implementation, this would use:
    # from mcp_ghidra import GhidraClient
    # client = GhidraClient(port=8192)
    # return client.functions.list(limit=10000)
    
    # For now, provide the structure that would be used
    return []


def analyze_unknown_functions_comprehensive():
    """
    Comprehensive analysis of unknown functions.
    
    Generates:
    1. symbol_addrs.txt entries (for compilation)
    2. Analysis report (for documentation)
    3. Decompilation targets (for m2c processing)
    4. Dependency graph (for understanding call chains)
    """
    
    print("=" * 80)
    print("Ghidra Comprehensive Symbol Analysis")
    print("=" * 80)
    print()
    
    # Analysis categories
    analysis = {
        "graphics": [],       # 0x80010000-0x80020000
        "entities": [],       # 0x80020000-0x80030000
        "mdec": [],          # 0x80030000-0x80040000
        "cd_blb": [],        # 0x80038000-0x8003A000
        "player": [],        # 0x80050000-0x80070000
        "level_asset": [],   # 0x80070000-0x80080000
        "audio": [],         # 0x8007C000-0x8007D000
        "entity_types": [],  # 0x80080000-0x80083000
        "main_system": [],   # 0x80082000-0x80090000
        "other": []
    }
    
    # Priority scoring
    priority_scores = {}
    
    # Load existing symbol_addrs.txt to avoid duplicates
    existing_symbols = set()
    symbol_file = Path(__file__).parent.parent / "symbol_addrs.txt"
    
    if symbol_file.exists():
        with open(symbol_file) as f:
            for line in f:
                if "=" in line and "0x" in line:
                    name = line.split("=")[0].strip()
                    existing_symbols.add(name)
    
    print(f"Loaded {len(existing_symbols)} existing symbols")
    print()
    
    return analysis, priority_scores


def generate_outputs(analysis: dict, priority_scores: dict):
    """Generate all output files."""
    
    output_dir = Path(__file__).parent.parent / "docs" / "analysis"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Symbol entries file
    symbols_file = output_dir / "unknown_symbols.txt"
    
    # 2. Analysis report
    report_file = output_dir / "unknown_functions_analysis.md"
    
    # 3. Decompilation queue
    decompile_file = output_dir / "decompilation_queue.txt"
    
    print(f"Generated files:")
    print(f"  - {symbols_file}")
    print(f"  - {report_file}")
    print(f"  - {decompile_file}")


if __name__ == "__main__":
    print("This is an enhanced version that would use the MCP client library")
    print("For now, use the REST API version: ghidra_symbol_report.py")
    print()
    print("To implement full MCP version:")
    print("  1. Install MCP Python client library")
    print("  2. Import GhidraClient from mcp_ghidra")
    print("  3. Use client.functions, client.decompile, etc.")
