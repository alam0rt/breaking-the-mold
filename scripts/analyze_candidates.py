#!/usr/bin/env python3
"""
Analyze functions from Ghidra to find good decompilation candidates.

This script queries Ghidra and ranks functions by "decompilability":
- Small size (20-100 bytes is ideal)
- Simple structure (few branches, no complex control flow)
- Named functions (not FUN_* or LAB_*)
- Not yet decompiled

Usage:
    python3 scripts/analyze_candidates.py --easy     # Show easiest functions first
    python3 scripts/analyze_candidates.py --medium   # Medium complexity
    python3 scripts/analyze_candidates.py --group GetLevel  # Functions matching pattern
"""

import argparse
import json
import sys
from pathlib import Path
from typing import List, Dict, Tuple
from urllib.request import urlopen

DEFAULT_PORT = 8192
SYMBOL_FILE = "symbol_addrs.txt"
SRC_DIR = "src"


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        return {"error": str(e)}


def get_all_functions(port: int) -> List[dict]:
    """Get all named functions from Ghidra."""
    result = ghidra_request(f"/functions?limit=10000", port)
    if result.get("success"):
        functions = result.get("result", result.get("functions", []))
        # Filter out auto-generated names
        return [f for f in functions 
                if not f.get("name", "").startswith("FUN_") 
                and not f.get("name", "").startswith("LAB_")]
    return []


def get_function_details(address: str, port: int) -> dict:
    """Get detailed info about a function including decompilation."""
    result = ghidra_request(f"/functions/{address}/decompile", port)
    if result.get("success"):
        return result
    return {}


def load_existing_symbols() -> set:
    """Load symbols from symbol_addrs.txt."""
    symbols = set()
    if Path(SYMBOL_FILE).exists():
        with open(SYMBOL_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('//'):
                    if '=' in line:
                        name = line.split('=')[0].strip()
                        symbols.add(name)
    return symbols


def load_decompiled_functions() -> set:
    """Check which functions already have C source files."""
    decompiled = set()
    src_path = Path(SRC_DIR)
    if src_path.exists():
        for c_file in src_path.glob("*.c"):
            # Assume filename = function name
            decompiled.add(c_file.stem)
    return decompiled


def estimate_complexity(func: dict, decompiled_code: str = None) -> Tuple[str, int]:
    """
    Estimate function complexity based on size and code analysis.
    Returns (complexity_level, score) where lower score = easier.
    """
    name = func.get("name", "")
    address = func.get("address", "0")
    
    # Get size from symbol_addrs.txt comment if available
    size = None
    try:
        with open(SYMBOL_FILE, 'r') as f:
            for line in f:
                if f"{name} =" in line and "size:" in line:
                    import re
                    match = re.search(r'size:0x([0-9A-Fa-f]+)', line)
                    if match:
                        size = int(match.group(1), 16)
                        break
    except:
        pass
    
    if not size:
        size = 100  # Assume medium size if unknown
    
    score = size  # Base score on size
    
    # Analyze decompiled code if available
    if decompiled_code:
        # Count branches
        branch_keywords = ['if', 'else', 'switch', 'case', 'for', 'while', 'do']
        branch_count = sum(decompiled_code.lower().count(kw) for kw in branch_keywords)
        score += branch_count * 10
        
        # Penalize complex operators
        if '->unk' in decompiled_code:
            score += 5  # Unknown struct fields
        if 'goto' in decompiled_code:
            score += 20  # Control flow complexity
        if decompiled_code.count('{') > 3:
            score += 10  # Nested blocks
    
    # Categorize
    if score < 30:
        return "EASY", score
    elif score < 80:
        return "MEDIUM", score
    elif score < 150:
        return "HARD", score
    else:
        return "VERY_HARD", score


def main():
    parser = argparse.ArgumentParser(
        description="Analyze functions to find good decompilation candidates"
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--easy", action="store_true",
                        help="Show only easy functions (< 30 bytes, simple)")
    parser.add_argument("--medium", action="store_true",
                        help="Show medium complexity functions")
    parser.add_argument("--hard", action="store_true",
                        help="Show hard functions (for challenge)")
    parser.add_argument("--group", type=str, default=None,
                        help="Filter by name pattern (e.g., 'GetLevel', 'EntityType')")
    parser.add_argument("--limit", type=int, default=20,
                        help="Maximum functions to show (default: 20)")
    parser.add_argument("--skip-decompiled", action="store_true",
                        help="Skip functions that already have C files")
    
    args = parser.parse_args()
    
    try:
        print("Fetching functions from Ghidra...", file=sys.stderr)
        functions = get_all_functions(args.port)
        
        if not functions:
            print("ERROR: No functions found in Ghidra", file=sys.stderr)
            return 1
        
        print(f"Found {len(functions)} named functions", file=sys.stderr)
        
        # Load existing data
        existing_symbols = load_existing_symbols()
        decompiled = load_decompiled_functions()
        
        print(f"Already decompiled: {len(decompiled)} functions", file=sys.stderr)
        
        # Filter and analyze
        candidates = []
        
        for func in functions:
            name = func.get("name", "")
            address = func.get("address", "0")
            
            # Skip if already decompiled
            if args.skip_decompiled and name in decompiled:
                continue
            
            # Apply group filter
            if args.group and args.group.lower() not in name.lower():
                continue
            
            # Estimate complexity
            complexity, score = estimate_complexity(func)
            
            # Apply complexity filter
            if args.easy and complexity != "EASY":
                continue
            if args.medium and complexity != "MEDIUM":
                continue
            if args.hard and complexity not in ["HARD", "VERY_HARD"]:
                continue
            
            candidates.append({
                "name": name,
                "address": address,
                "complexity": complexity,
                "score": score,
                "in_symbols": name in existing_symbols,
                "decompiled": name in decompiled
            })
        
        # Sort by score (easiest first)
        candidates.sort(key=lambda x: x["score"])
        
        # Display results
        print(f"\n{'='*80}")
        print(f"DECOMPILATION CANDIDATES")
        print(f"{'='*80}\n")
        
        if not candidates:
            print("No candidates found matching criteria")
            return 0
        
        print(f"Found {len(candidates)} candidates (showing top {args.limit}):\n")
        print(f"{'Function':<40} {'Address':<12} {'Complexity':<12} {'Score':<8} Status")
        print(f"{'-'*80}")
        
        for candidate in candidates[:args.limit]:
            status_parts = []
            if candidate["in_symbols"]:
                status_parts.append("in symbols")
            if candidate["decompiled"]:
                status_parts.append("DECOMPILED")
            status = ", ".join(status_parts) if status_parts else "new"
            
            print(f"{candidate['name']:<40} "
                  f"{candidate['address']:<12} "
                  f"{candidate['complexity']:<12} "
                  f"{candidate['score']:<8} "
                  f"{status}")
        
        # Summary
        print(f"\n{'-'*80}")
        print(f"Total candidates: {len(candidates)}")
        by_complexity = {}
        for c in candidates:
            by_complexity[c['complexity']] = by_complexity.get(c['complexity'], 0) + 1
        
        print(f"By complexity:")
        for level in ["EASY", "MEDIUM", "HARD", "VERY_HARD"]:
            if level in by_complexity:
                print(f"  {level}: {by_complexity[level]}")
        
        # Suggest batch command
        if candidates:
            print(f"\nSuggested command to decompile top candidates:")
            names = " ".join([c["name"] for c in candidates[:5]])
            if args.group:
                print(f"  python3 scripts/auto_decompile.py --pattern '{args.group}*' --verify --limit 10")
            else:
                print(f"  # Manually decompile one at a time:")
                for c in candidates[:5]:
                    print(f"  python3 scripts/auto_decompile_function.py {c['name']} --verify")
        
        return 0
    
    except KeyboardInterrupt:
        print("\nInterrupted by user", file=sys.stderr)
        return 130
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
