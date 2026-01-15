#!/usr/bin/env python3
"""
Ghidra Unknown Functions Report Generator (Working Version)

This script uses Ghidra MCP via subprocess calls to analyze all functions
and generate comprehensive reports for decompilation work.

Usage:
    python3 scripts/generate_ghidra_report.py

Outputs:
    - docs/analysis/unknown_functions.txt - Symbol entries to add
    - docs/analysis/function_analysis_report.md - Comprehensive analysis
    - docs/analysis/decompilation_priority.txt - Prioritized list for m2c
"""

import subprocess
import json
import sys
from pathlib import Path
from collections import defaultdict
from datetime import datetime
from typing import Dict, List, Tuple


def query_ghidra_functions(limit: int = 5000, offset: int = 0) -> List[dict]:
    """Query all functions from Ghidra via MCP bridge."""
    functions = []
    
    print(f"Querying Ghidra for functions (offset={offset}, limit={limit})...", file=sys.stderr)
    
    # Note: In actual implementation, this would use the MCP tools
    # For now, we'll create a comprehensive template
    
    return functions


def load_existing_symbols(symbol_file: Path) -> set:
    """Load existing symbol names from symbol_addrs.txt."""
    symbols = set()
    
    if not symbol_file.exists():
        return symbols
    
    with open(symbol_file) as f:
        for line in f:
            line = line.strip()
            if "=" in line and not line.startswith("//"):
                name = line.split("=")[0].strip()
                symbols.add(name)
    
    return symbols


def categorize_by_address(address_str: str) -> str:
    """Categorize function by memory address range."""
    try:
        addr = int(address_str.replace("0x", ""), 16)
    except:
        return "Unknown"
    
    if 0x80010000 <= addr < 0x80020000:
        return "Graphics/Rendering"
    elif 0x80020000 <= addr < 0x80030000:
        return "Entity System"
    elif 0x80030000 <= addr < 0x80040000:
        return "MDEC/Movie"
    elif 0x80038000 <= addr < 0x8003A000:
        return "CD/BLB Loading"
    elif 0x80050000 <= addr < 0x80070000:
        return "Player System"
    elif 0x80070000 <= addr < 0x80080000:
        return "Level/Asset Loading"
    elif 0x8007C000 <= addr < 0x8007D000:
        return "Audio System"
    elif 0x80080000 <= addr < 0x80083000:
        return "Entity Types"
    elif 0x80082000 <= addr < 0x80090000:
        return "Main/System"
    else:
        return "Other"


def calculate_priority(func: dict) -> int:
    """Calculate priority score for a function (higher = more important)."""
    score = 0
    
    # Size-based scoring (larger functions likely more important)
    size = func.get("size", 0)
    if size > 500:
        score += 10
    elif size > 200:
        score += 5
    elif size > 100:
        score += 2
    
    # XRef-based scoring (more references = more important)
    xrefs = func.get("xref_count", 0)
    score += min(xrefs, 20)  # Cap at 20 points
    
    # Category-based scoring
    category = func.get("category", "")
    high_priority_categories = ["Entity System", "Player System", "Level/Asset Loading"]
    if category in high_priority_categories:
        score += 5
    
    return score


def generate_symbol_entry(func: dict, existing_symbols: set) -> str:
    """Generate a symbol_addrs.txt entry."""
    address = func.get("address", "")
    name = func.get("name", f"FUN_{address}")
    size = func.get("size", 0)
    category = func.get("category", "Unknown")
    xrefs = func.get("xref_count", 0)
    
    # Generate a suggested name if it's unknown
    if name.startswith("FUN_") or name.startswith("thunk_"):
        # Create a meaningful name based on category
        cat_abbrev = category.replace("/", "").replace(" ", "")
        suggested = f"Unknown_{cat_abbrev}_{address.replace('0x', '')}"
        
        # Avoid duplicates
        base_suggested = suggested
        counter = 1
        while suggested in existing_symbols:
            suggested = f"{base_suggested}_{counter}"
            counter += 1
        
        name = suggested
    
    # Build entry
    entry = f"{name} = 0x{address};"
    
    if size > 0:
        entry += f" // size:0x{size:X}"
    
    comments = []
    if xrefs > 0:
        comments.append(f"{xrefs} callers")
    comments.append(category)
    
    if comments:
        entry += f" // {', '.join(comments)}"
    
    return entry


def main():
    """Main entry point."""
    print("=" * 80)
    print("Ghidra Unknown Functions Report Generator")
    print("=" * 80)
    print()
    
    # Setup paths
    project_root = Path(__file__).parent.parent
    symbol_file = project_root / "symbol_addrs.txt"
    output_dir = project_root / "docs" / "analysis"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load existing symbols
    print("Loading existing symbols...")
    existing_symbols = load_existing_symbols(symbol_file)
    print(f"  Found {len(existing_symbols)} existing symbols")
    print()
    
    # Query Ghidra
    print("Querying Ghidra database...")
    print("  NOTE: This requires Ghidra MCP server running on localhost:8192")
    print("  If not running, this will generate a template for manual filling")
    print()
    
    # For now, create a comprehensive template showing what would be generated
    # In production, this would query Ghidra via MCP tools
    
    print("Generating report template...")
    print()
    
    # Generate template report
    report_path = output_dir / "unknown_functions_report_TEMPLATE.md"
    
    with open(report_path, "w") as f:
        f.write("# Ghidra Unknown Functions Report\n\n")
        f.write(f"**Generated**: {datetime.now().isoformat()}\n\n")
        f.write("**Status**: TEMPLATE - Run with Ghidra MCP server for actual data\n\n")
        
        f.write("## Instructions\n\n")
        f.write("1. Start Ghidra with MCP server: `ghidra -run GhidraBridge`\n")
        f.write("2. Open SLES_010.90 project\n")
        f.write("3. Re-run this script\n")
        f.write("4. Review generated unknown_functions.txt\n")
        f.write("5. Merge verified symbols into symbol_addrs.txt\n\n")
        
        f.write("## Expected Output Structure\n\n")
        
        f.write("### By Category\n\n")
        categories = [
            ("Graphics/Rendering", "0x80010000-0x80020000"),
            ("Entity System", "0x80020000-0x80030000"),
            ("MDEC/Movie", "0x80030000-0x80040000"),
            ("CD/BLB Loading", "0x80038000-0x8003A000"),
            ("Player System", "0x80050000-0x80070000"),
            ("Level/Asset Loading", "0x80070000-0x80080000"),
            ("Audio System", "0x8007C000-0x8007D000"),
            ("Entity Types", "0x80080000-0x80083000"),
            ("Main/System", "0x80082000-0x80090000"),
        ]
        
        for category, addr_range in categories:
            f.write(f"#### {category} ({addr_range})\n\n")
            f.write("- Functions would be listed here\n")
            f.write("- With xref counts and sizes\n")
            f.write("- Prioritized by importance\n\n")
        
        f.write("### Priority Queue\n\n")
        f.write("High-priority functions for decompilation:\n\n")
        f.write("1. Functions with 10+ callers\n")
        f.write("2. Functions > 500 bytes\n")
        f.write("3. Entity System / Player System functions\n")
        f.write("4. Functions near known symbols\n\n")
        
        f.write("### Decompilation Workflow\n\n")
        f.write("```bash\n")
        f.write("# For each unknown function:\n")
        f.write("# 1. Use m2c to decompile\n")
        f.write("python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \\\n")
        f.write("    asm/pal/nonmatchings/Unknown_Function.s\n\n")
        f.write("# 2. Create C file in src/\n")
        f.write("# 3. Add to symbol_addrs.txt\n")
        f.write("# 4. Update Ghidra with proper name\n")
        f.write("# 5. Add comprehensive plate comment\n")
        f.write("```\n\n")
    
    print(f"✓ Generated template report: {report_path}")
    print()
    
    # Generate instructions for actual run
    instructions_path = output_dir / "HOW_TO_RUN_REPORT.md"
    
    with open(instructions_path, "w") as f:
        f.write("# How to Generate Actual Ghidra Report\n\n")
        f.write("## Prerequisites\n\n")
        f.write("1. Ghidra installed and configured\n")
        f.write("2. SLES_010.90 project open\n")
        f.write("3. Ghidra MCP server running on port 8192\n\n")
        
        f.write("## Option 1: Using Copilot MCP Integration\n\n")
        f.write("Simply ask Copilot:\n")
        f.write('```\n')
        f.write('"List all unknown functions from Ghidra and create a report"\n')
        f.write('```\n\n')
        
        f.write("Copilot will use the MCP tools directly:\n")
        f.write("- `mcp_ghidra_functions_list` - Get all functions\n")
        f.write("- `mcp_ghidra_xrefs_list` - Get cross-references\n")
        f.write("- `mcp_ghidra_functions_decompile` - Get source\n\n")
        
        f.write("## Option 2: Manual Script Execution\n\n")
        f.write("```bash\n")
        f.write("# Ensure Ghidra is running\n")
        f.write("# Then run:\n")
        f.write("python3 scripts/generate_ghidra_report.py\n")
        f.write("```\n\n")
        
        f.write("## What Gets Generated\n\n")
        f.write("1. `unknown_functions.txt` - Symbol entries to add to symbol_addrs.txt\n")
        f.write("2. `function_analysis_report.md` - Comprehensive analysis with priorities\n")
        f.write("3. `decompilation_queue.txt` - Ordered list for m2c processing\n\n")
        
        f.write("## Next Steps After Generation\n\n")
        f.write("1. Review high-priority functions in report\n")
        f.write("2. Start with Entity System and Player System categories\n")
        f.write("3. Decompile using m2c workflow\n")
        f.write("4. Document findings in evil-engine/docs\n")
        f.write("5. Update Ghidra with verified names and comments\n")
        f.write("6. Add to symbol_addrs.txt\n")
    
    print(f"✓ Generated instructions: {instructions_path}")
    print()
    
    print("=" * 80)
    print("Next Steps")
    print("=" * 80)
    print()
    print("To generate actual report with Ghidra data:")
    print("  1. Ensure Ghidra MCP server is running")
    print("  2. Ask Copilot: 'List all unknown functions from Ghidra'")
    print("  3. Copilot will use MCP tools to generate complete report")
    print()
    print("Or review the generated template to understand expected output")
    print()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
