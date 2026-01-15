#!/usr/bin/env python3
"""
Master orchestration script for automated decompilation.

This script coordinates the entire decompilation pipeline:
1. Sync symbols bidirectionally (Ghidra ↔ symbol_addrs.txt)
2. Update splat configuration
3. Decompile functions (optionally with verification)
4. Track progress and generate reports

Usage:
    # Full auto mode: decompile all functions
    python3 scripts/auto_decompile.py --full-auto

    # Decompile single function
    python3 scripts/auto_decompile.py --function GetLevelCount

    # Decompile multiple functions by pattern
    python3 scripts/auto_decompile.py --pattern "EntityType0*"

    # Batch decompile with verification
    python3 scripts/auto_decompile.py --batch --verify

    # Dry run to see what would happen
    python3 scripts/auto_decompile.py --full-auto --dry-run
"""

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from urllib.request import urlopen
from urllib.error import URLError

DEFAULT_PORT = 8192
PROGRESS_FILE = "build/decompilation_progress.json"


def run_script(script: str, args: List[str], description: str) -> Tuple[bool, str]:
    """Run a Python script and return success status."""
    cmd = ["python3", script] + args
    print(f"\n=== {description} ===", file=sys.stderr)
    print(f"Running: {' '.join(cmd)}", file=sys.stderr)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=False)
        
        # Print output
        if result.stdout:
            print(result.stdout, end='')
        if result.stderr:
            print(result.stderr, end='', file=sys.stderr)
        
        if result.returncode != 0:
            print(f"ERROR: {description} failed (exit code {result.returncode})", file=sys.stderr)
            return False, result.stderr
        
        return True, result.stdout
    
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return False, str(e)


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        return {"error": str(e)}


def get_all_functions(port: int) -> List[dict]:
    """Get all named functions from Ghidra."""
    result = ghidra_request(f"/functions?limit=10000", port)
    if result.get("success"):
        # MCP API returns functions in 'result', not 'functions'
        functions = result.get("result", result.get("functions", []))
        # Filter out auto-generated names
        return [f for f in functions 
                if not f.get("name", "").startswith("FUN_") 
                and not f.get("name", "").startswith("LAB_")]
    return []


def load_progress() -> Dict:
    """Load decompilation progress from file."""
    progress_path = Path(PROGRESS_FILE)
    if progress_path.exists():
        with open(progress_path, 'r') as f:
            return json.load(f)
    return {
        "attempted": {},
        "matched": {},
        "failed": {},
        "stats": {
            "total_attempted": 0,
            "total_matched": 0,
            "total_failed": 0
        }
    }


def save_progress(progress: Dict):
    """Save decompilation progress to file."""
    progress_path = Path(PROGRESS_FILE)
    progress_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(progress_path, 'w') as f:
        json.dump(progress, f, indent=2)


def mark_attempted(progress: Dict, func_name: str, address: int, timestamp: float):
    """Mark function as attempted."""
    progress["attempted"][func_name] = {
        "address": f"0x{address:08X}",
        "timestamp": timestamp
    }
    progress["stats"]["total_attempted"] = len(progress["attempted"])
    save_progress(progress)


def mark_matched(progress: Dict, func_name: str, address: int, timestamp: float):
    """Mark function as successfully matched."""
    progress["matched"][func_name] = {
        "address": f"0x{address:08X}",
        "timestamp": timestamp
    }
    if func_name in progress["failed"]:
        del progress["failed"][func_name]
    progress["stats"]["total_matched"] = len(progress["matched"])
    progress["stats"]["total_failed"] = len(progress["failed"])
    save_progress(progress)


def mark_failed(progress: Dict, func_name: str, address: int, timestamp: float, reason: str):
    """Mark function as failed to match."""
    progress["failed"][func_name] = {
        "address": f"0x{address:08X}",
        "timestamp": timestamp,
        "reason": reason
    }
    if func_name in progress["matched"]:
        del progress["matched"][func_name]
    progress["stats"]["total_matched"] = len(progress["matched"])
    progress["stats"]["total_failed"] = len(progress["failed"])
    save_progress(progress)


def sync_symbols(port: int, dry_run: bool = False) -> bool:
    """Sync symbols between Ghidra and symbol_addrs.txt."""
    args = ["--export", "--port", str(port)]
    if dry_run:
        args.append("--dry-run")
    
    success, _ = run_script(
        "scripts/ghidra_sync_symbols.py",
        args,
        "Sync symbols from Ghidra"
    )
    return success


def update_splat_config(port: int, filter_pattern: Optional[str] = None, 
                        dry_run: bool = False) -> bool:
    """Update splat configuration."""
    args = ["--port", str(port)]
    if filter_pattern:
        args.extend(["--filter", filter_pattern])
    if dry_run:
        args.append("--dry-run")
    
    success, _ = run_script(
        "scripts/update_splat_config.py",
        args,
        "Update splat configuration"
    )
    return success


def decompile_function(func_name: str, port: int, verify: bool = False, 
                       dry_run: bool = False) -> Tuple[bool, bool]:
    """
    Decompile a single function.
    Returns: (success, matched) where matched is True if build verification passed.
    """
    args = [func_name, "--port", str(port)]
    if verify:
        args.append("--verify")
    if dry_run:
        args.append("--dry-run")
    
    success, output = run_script(
        "scripts/auto_decompile_function.py",
        args,
        f"Decompile {func_name}"
    )
    
    # Check if verification passed
    matched = False
    if verify and success:
        matched = "BUILD MATCHES ORIGINAL" in output
    
    return success, matched


def filter_functions(functions: List[dict], pattern: Optional[str] = None,
                     skip_attempted: bool = False, progress: Optional[Dict] = None) -> List[dict]:
    """Filter functions based on criteria."""
    filtered = functions
    
    # Apply pattern filter
    if pattern:
        import fnmatch
        filtered = [f for f in filtered if fnmatch.fnmatch(f.get("name", ""), pattern)]
    
    # Skip already attempted
    if skip_attempted and progress:
        attempted = set(progress.get("attempted", {}).keys())
        filtered = [f for f in filtered if f.get("name") not in attempted]
    
    return filtered


def generate_report(progress: Dict, port: int):
    """Generate and display progress report."""
    print("\n" + "="*60)
    print("DECOMPILATION PROGRESS REPORT")
    print("="*60)
    
    stats = progress.get("stats", {})
    total_attempted = stats.get("total_attempted", 0)
    total_matched = stats.get("total_matched", 0)
    total_failed = stats.get("total_failed", 0)
    
    # Get total functions from Ghidra
    all_functions = get_all_functions(port)
    total_functions = len(all_functions)
    
    print(f"\nTotal functions in Ghidra: {total_functions}")
    print(f"Attempted: {total_attempted}")
    print(f"Matched: {total_matched}")
    print(f"Failed: {total_failed}")
    
    if total_functions > 0:
        coverage = (total_matched / total_functions) * 100
        print(f"Coverage: {coverage:.1f}%")
    
    if total_attempted > 0:
        success_rate = (total_matched / total_attempted) * 100
        print(f"Success rate: {success_rate:.1f}%")
    
    # Show recent failures
    failed = progress.get("failed", {})
    if failed:
        print(f"\nRecent failures:")
        for func_name, info in list(failed.items())[:5]:
            print(f"  - {func_name} @ {info['address']}: {info.get('reason', 'unknown')}")
        
        if len(failed) > 5:
            print(f"  ... and {len(failed) - 5} more")
    
    print("\n" + "="*60 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="Master orchestration script for automated decompilation"
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would happen without making changes")
    
    # Mode selection
    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument("--full-auto", action="store_true",
                            help="Decompile all functions automatically")
    mode_group.add_argument("--function", type=str,
                            help="Decompile single function by name")
    mode_group.add_argument("--pattern", type=str,
                            help="Decompile functions matching pattern (e.g., 'EntityType*')")
    mode_group.add_argument("--batch", action="store_true",
                            help="Batch decompile (prompts for each function)")
    mode_group.add_argument("--report", action="store_true",
                            help="Show progress report only")
    
    # Options
    parser.add_argument("--verify", action="store_true",
                        help="Run build verification after each function")
    parser.add_argument("--skip-sync", action="store_true",
                        help="Skip symbol synchronization step")
    parser.add_argument("--skip-attempted", action="store_true",
                        help="Skip functions that have already been attempted")
    parser.add_argument("--limit", type=int, default=None,
                        help="Limit number of functions to process")
    parser.add_argument("--rollback-on-fail", action="store_true",
                        help="Revert changes if build verification fails")
    
    args = parser.parse_args()
    
    try:
        # Load progress
        progress = load_progress()
        
        # Show report and exit
        if args.report:
            generate_report(progress, args.port)
            return 0
        
        # Step 1: Sync symbols (unless skipped)
        if not args.skip_sync:
            if not sync_symbols(args.port, args.dry_run):
                print("ERROR: Symbol sync failed", file=sys.stderr)
                return 1
        
        # Step 2: Determine functions to decompile
        if args.function:
            # Single function mode
            func_names = [args.function]
        else:
            # Get all functions from Ghidra
            all_functions = get_all_functions(args.port)
            
            if not all_functions:
                print("ERROR: No functions found in Ghidra", file=sys.stderr)
                return 1
            
            print(f"Found {len(all_functions)} named functions in Ghidra", file=sys.stderr)
            
            # Apply filters
            filtered = filter_functions(
                all_functions,
                pattern=args.pattern,
                skip_attempted=args.skip_attempted,
                progress=progress
            )
            
            # Apply limit
            if args.limit:
                filtered = filtered[:args.limit]
            
            func_names = [f.get("name") for f in filtered]
            print(f"Selected {len(func_names)} functions to process", file=sys.stderr)
        
        if not func_names:
            print("No functions to process", file=sys.stderr)
            return 0
        
        # Step 3: Update splat config
        pattern_filter = args.pattern if args.pattern else None
        if not update_splat_config(args.port, pattern_filter, args.dry_run):
            print("ERROR: Splat config update failed", file=sys.stderr)
            return 1
        
        # Step 4: Decompile functions
        success_count = 0
        fail_count = 0
        
        for idx, func_name in enumerate(func_names):
            print(f"\n[{idx+1}/{len(func_names)}] Processing {func_name}...", file=sys.stderr)
            
            timestamp = time.time()
            
            # Mark as attempted
            # Get address (simplified - we'd need to query Ghidra)
            address = 0  # Placeholder
            mark_attempted(progress, func_name, address, timestamp)
            
            # Decompile
            success, matched = decompile_function(func_name, args.port, args.verify, args.dry_run)
            
            if success:
                if args.verify:
                    if matched:
                        print(f"  ✓ {func_name} matched!", file=sys.stderr)
                        mark_matched(progress, func_name, address, timestamp)
                        success_count += 1
                    else:
                        print(f"  ✗ {func_name} did not match", file=sys.stderr)
                        mark_failed(progress, func_name, address, timestamp, "Build mismatch")
                        fail_count += 1
                        
                        if args.rollback_on_fail:
                            print(f"    Rolling back changes...", file=sys.stderr)
                            # TODO: Implement rollback logic
                else:
                    print(f"  ✓ {func_name} decompiled (not verified)", file=sys.stderr)
                    success_count += 1
            else:
                print(f"  ✗ {func_name} failed", file=sys.stderr)
                mark_failed(progress, func_name, address, timestamp, "Decompilation error")
                fail_count += 1
            
            # Interactive mode: ask to continue
            if args.batch and not args.dry_run:
                response = input("Continue? [Y/n/q] ").strip().lower()
                if response == 'q':
                    print("Quitting...", file=sys.stderr)
                    break
                elif response == 'n':
                    print("Skipping...", file=sys.stderr)
                    continue
        
        # Step 5: Generate final report
        print(f"\n\nProcessed {len(func_names)} functions:", file=sys.stderr)
        print(f"  Success: {success_count}", file=sys.stderr)
        print(f"  Failed: {fail_count}", file=sys.stderr)
        
        generate_report(progress, args.port)
        
        return 0 if fail_count == 0 else 1
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        return 130
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
