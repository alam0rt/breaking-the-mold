#!/usr/bin/env python3
"""
Ghidra Symbol Report Generator
Analyzes Ghidra database and generates symbol_addrs.txt entries for unknown functions.

This script:
1. Queries all functions in Ghidra
2. Identifies unnamed functions (FUN_*, thunk_FUN_*, etc.)
3. Analyzes function characteristics (size, callers, xrefs)
4. Generates symbol_addrs.txt format output
5. Provides analysis report for prioritization

Usage:
    python3 scripts/ghidra_symbol_report.py [--output symbol_addrs_new.txt] [--min-size 16]
"""

import argparse
import json
import sys
from collections import defaultdict
from typing import Dict, List, Tuple, Optional
import subprocess


class GhidraAnalyzer:
    """Analyze Ghidra database for unknown functions."""
    
    def __init__(self, ghidra_port: int = 8192):
        self.ghidra_port = ghidra_port
        self.base_url = f"http://localhost:{ghidra_port}/api/v1"
        
    def query_ghidra(self, endpoint: str) -> dict:
        """Query Ghidra MCP API."""
        try:
            result = subprocess.run(
                ["curl", "-s", f"{self.base_url}/{endpoint}"],
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode == 0 and result.stdout:
                return json.loads(result.stdout)
            return {}
        except Exception as e:
            print(f"Error querying {endpoint}: {e}", file=sys.stderr)
            return {}
    
    def get_all_functions(self, offset: int = 0, limit: int = 1000) -> List[dict]:
        """Retrieve all functions from Ghidra."""
        functions = []
        
        while True:
            data = self.query_ghidra(f"functions?offset={offset}&limit={limit}")
            
            if not data or "functions" not in data:
                break
            
            batch = data["functions"]
            if not batch:
                break
            
            functions.extend(batch)
            
            # Check if we got all functions
            if len(batch) < limit:
                break
            
            offset += limit
            print(f"Retrieved {len(functions)} functions...", file=sys.stderr)
        
        return functions
    
    def get_function_xrefs(self, address: str) -> dict:
        """Get cross-references to a function."""
        return self.query_ghidra(f"xrefs?to_addr={address}&limit=100")
    
    def is_unknown_function(self, func: dict) -> bool:
        """Check if function has a default/unknown name."""
        name = func.get("name", "")
        
        # Unknown function patterns
        unknown_patterns = [
            "FUN_",
            "thunk_FUN_",
            "UndefinedFunction_",
            "DAT_",
        ]
        
        return any(name.startswith(pattern) for pattern in unknown_patterns)
    
    def categorize_function(self, func: dict, xrefs: dict) -> str:
        """Categorize function by address range and characteristics."""
        address = func.get("address", "")
        size = func.get("length", 0)
        xref_count = len(xrefs.get("results", []))
        
        # Address range categorization
        addr_int = int(address, 16)
        
        if 0x80010000 <= addr_int < 0x80020000:
            return "Graphics/Rendering"
        elif 0x80020000 <= addr_int < 0x80030000:
            return "Entity System"
        elif 0x80030000 <= addr_int < 0x8004000:
            return "MDEC/Movie"
        elif 0x80038000 <= addr_int < 0x8003A000:
            return "CD/BLB Loading"
        elif 0x80050000 <= addr_int < 0x80070000:
            return "Player System"
        elif 0x80070000 <= addr_int < 0x80080000:
            return "Level/Asset Loading"
        elif 0x8007C000 <= addr_int < 0x8007D000:
            return "Audio System"
        elif 0x80080000 <= addr_int < 0x80083000:
            return "Entity Types"
        elif 0x80082000 <= addr_int < 0x80090000:
            return "Main/System"
        else:
            return "Unknown/Other"
    
    def analyze_function(self, func: dict) -> dict:
        """Analyze a single function."""
        address = func.get("address", "")
        name = func.get("name", "")
        size = func.get("length", 0)
        
        # Get cross-references
        xrefs = self.get_function_xrefs(address)
        xref_count = len(xrefs.get("results", []))
        
        category = self.categorize_function(func, xrefs)
        
        return {
            "address": address,
            "name": name,
            "size": size,
            "xref_count": xref_count,
            "category": category,
            "is_thunk": name.startswith("thunk_"),
            "callers": [x.get("fromAddr") for x in xrefs.get("results", [])[:5]]
        }


def generate_symbol_entry(func_info: dict, comment: str = "") -> str:
    """Generate symbol_addrs.txt format entry."""
    address = func_info["address"]
    name = func_info["name"]
    size = func_info["size"]
    
    # Suggest a better name based on category
    suggested_name = name
    if func_info["is_thunk"]:
        suggested_name = f"Thunk_{address}"
    else:
        suggested_name = f"Unknown_{func_info['category'].replace('/', '_').replace(' ', '')}_{address}"
    
    entry = f"{suggested_name} = 0x{address};"
    
    if size > 0:
        entry += f" // size:0x{size:X}"
    
    if comment:
        entry += f" // {comment}"
    elif func_info["xref_count"] > 0:
        entry += f" // {func_info['xref_count']} callers"
    
    return entry


def main():
    parser = argparse.ArgumentParser(
        description="Generate Ghidra symbol report for unknown functions"
    )
    parser.add_argument(
        "--output", "-o",
        default="symbol_addrs_unknown.txt",
        help="Output file for symbol entries"
    )
    parser.add_argument(
        "--report", "-r",
        default="ghidra_function_report.md",
        help="Output file for analysis report"
    )
    parser.add_argument(
        "--min-size", "-s",
        type=int,
        default=16,
        help="Minimum function size to include (bytes)"
    )
    parser.add_argument(
        "--min-xrefs", "-x",
        type=int,
        default=0,
        help="Minimum number of cross-references"
    )
    parser.add_argument(
        "--port", "-p",
        type=int,
        default=8192,
        help="Ghidra MCP server port"
    )
    
    args = parser.parse_args()
    
    print("=" * 80)
    print("Ghidra Symbol Report Generator")
    print("=" * 80)
    print()
    
    # Initialize analyzer
    analyzer = GhidraAnalyzer(ghidra_port=args.port)
    
    # Get all functions
    print("Retrieving functions from Ghidra...")
    all_functions = analyzer.get_all_functions()
    print(f"Retrieved {len(all_functions)} total functions")
    print()
    
    # Filter unknown functions
    print("Analyzing unknown functions...")
    unknown_functions = []
    
    for func in all_functions:
        if analyzer.is_unknown_function(func):
            func_info = analyzer.analyze_function(func)
            
            # Apply filters
            if func_info["size"] >= args.min_size and func_info["xref_count"] >= args.min_xrefs:
                unknown_functions.append(func_info)
    
    print(f"Found {len(unknown_functions)} unknown functions")
    print()
    
    # Group by category
    by_category = defaultdict(list)
    for func in unknown_functions:
        by_category[func["category"]].append(func)
    
    # Sort by xref count (most called first)
    for category in by_category:
        by_category[category].sort(key=lambda f: f["xref_count"], reverse=True)
    
    # Generate symbol_addrs.txt output
    print(f"Writing symbol entries to {args.output}...")
    with open(args.output, "w") as f:
        f.write("// =============================================================================\n")
        f.write("// Unknown Functions - Generated from Ghidra Analysis\n")
        f.write(f"// Generated: {__import__('datetime').datetime.now().isoformat()}\n")
        f.write(f"// Total unknown functions: {len(unknown_functions)}\n")
        f.write("// =============================================================================\n\n")
        
        for category in sorted(by_category.keys()):
            funcs = by_category[category]
            f.write(f"// =============================================================================\n")
            f.write(f"// {category} ({len(funcs)} functions)\n")
            f.write("// =============================================================================\n")
            
            for func in funcs:
                entry = generate_symbol_entry(func)
                f.write(f"{entry}\n")
            
            f.write("\n")
    
    print(f"✓ Wrote {len(unknown_functions)} symbol entries")
    print()
    
    # Generate analysis report
    print(f"Writing analysis report to {args.report}...")
    with open(args.report, "w") as f:
        f.write("# Ghidra Unknown Functions Report\n\n")
        f.write(f"**Generated**: {__import__('datetime').datetime.now().isoformat()}\n\n")
        f.write(f"**Total Functions**: {len(all_functions)}\n")
        f.write(f"**Unknown Functions**: {len(unknown_functions)}\n")
        f.write(f"**Known Functions**: {len(all_functions) - len(unknown_functions)}\n\n")
        
        f.write("## Summary by Category\n\n")
        f.write("| Category | Count | Avg Size | Total XRefs |\n")
        f.write("|----------|-------|----------|-------------|\n")
        
        for category in sorted(by_category.keys()):
            funcs = by_category[category]
            avg_size = sum(f["size"] for f in funcs) / len(funcs) if funcs else 0
            total_xrefs = sum(f["xref_count"] for f in funcs)
            f.write(f"| {category} | {len(funcs)} | {avg_size:.0f} | {total_xrefs} |\n")
        
        f.write("\n## Priority Analysis\n\n")
        f.write("### High Priority (Most Referenced)\n\n")
        
        high_priority = sorted(unknown_functions, key=lambda x: x["xref_count"], reverse=True)[:20]
        
        f.write("| Address | Name | Size | XRefs | Category |\n")
        f.write("|---------|------|------|-------|----------|\n")
        
        for func in high_priority:
            f.write(f"| 0x{func['address']} | `{func['name']}` | {func['size']} | {func['xref_count']} | {func['category']} |\n")
        
        f.write("\n### Large Functions (Likely Important)\n\n")
        
        large_funcs = sorted(unknown_functions, key=lambda x: x["size"], reverse=True)[:20]
        
        f.write("| Address | Name | Size | XRefs | Category |\n")
        f.write("|---------|------|------|-------|----------|\n")
        
        for func in large_funcs:
            f.write(f"| 0x{func['address']} | `{func['name']}` | {func['size']} | {func['xref_count']} | {func['category']} |\n")
        
        f.write("\n## Detailed Function List\n\n")
        
        for category in sorted(by_category.keys()):
            f.write(f"### {category}\n\n")
            
            funcs = by_category[category]
            f.write("| Address | Name | Size | XRefs |\n")
            f.write("|---------|------|------|-------|\n")
            
            for func in funcs[:50]:  # Limit to first 50 per category
                f.write(f"| 0x{func['address']} | `{func['name']}` | {func['size']} | {func['xref_count']} |\n")
            
            if len(funcs) > 50:
                f.write(f"\n*...and {len(funcs) - 50} more*\n")
            
            f.write("\n")
    
    print(f"✓ Wrote analysis report")
    print()
    
    # Print summary
    print("=" * 80)
    print("Summary")
    print("=" * 80)
    print()
    print(f"Total functions in Ghidra: {len(all_functions)}")
    print(f"Unknown functions found: {len(unknown_functions)}")
    print(f"Known/Named functions: {len(all_functions) - len(unknown_functions)}")
    print()
    print("Top 5 categories by function count:")
    for i, (category, funcs) in enumerate(sorted(by_category.items(), key=lambda x: len(x[1]), reverse=True)[:5], 1):
        print(f"  {i}. {category}: {len(funcs)} functions")
    print()
    print(f"Output files:")
    print(f"  - Symbol entries: {args.output}")
    print(f"  - Analysis report: {args.report}")
    print()
    print("Next steps:")
    print("  1. Review analysis report for prioritization")
    print("  2. Merge symbol entries into main symbol_addrs.txt")
    print("  3. Use evil-engine docs to identify function purposes")
    print("  4. Update Ghidra with verified names and comments")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
