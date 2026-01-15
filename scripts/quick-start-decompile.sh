#!/bin/bash
# Quick start script for the automated decompilation system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

echo "=========================================="
echo "Automated Decompilation System - Quick Start"
echo "=========================================="
echo ""

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v python3 &> /dev/null; then
    echo "❌ python3 not found. Please install Python 3.8+"
    exit 1
fi

if ! python3 -c "import yaml" 2>/dev/null; then
    echo "❌ PyYAML not installed. Installing..."
    pip3 install pyyaml --user
fi

if ! command -v cpp &> /dev/null; then
    echo "❌ cpp (C preprocessor) not found. Please install build-essential or similar"
    exit 1
fi

echo "✅ Prerequisites OK"
echo ""

# Check if Ghidra MCP is running
echo "Checking Ghidra MCP server..."
if curl -s -o /dev/null -w "%{http_code}" http://localhost:8192/functions 2>/dev/null | grep -q "200"; then
    GHIDRA_RUNNING=true
    echo "✅ Ghidra MCP server is running on port 8192"
    
    # Get function count
    FUNC_COUNT=$(curl -s http://localhost:8192/functions?limit=10000 | python3 -c "import sys, json; data=json.load(sys.stdin); print(len(data.get('functions', [])))" 2>/dev/null || echo "0")
    echo "   Found $FUNC_COUNT functions in Ghidra"
else
    GHIDRA_RUNNING=false
    echo "⚠️  Ghidra MCP server not detected on port 8192"
    echo "   Start it with: python3 scripts/ghidra_mcp_server.py"
fi
echo ""

# Generate context file if needed
if [ ! -f "ctx.c" ]; then
    echo "Generating ctx.c context file..."
    make context
    echo ""
else
    echo "✅ ctx.c already exists"
    echo ""
fi

# Show options
echo "=========================================="
echo "What would you like to do?"
echo "=========================================="
echo ""
echo "1) Validate symbol sync (check Ghidra ↔ symbol_addrs.txt)"
echo "2) Export symbols from Ghidra to symbol_addrs.txt"
echo "3) Import symbols from symbol_addrs.txt to Ghidra"
echo "4) Decompile a single function (with verification)"
echo "5) Batch decompile functions by pattern"
echo "6) Show decompilation progress report"
echo "7) Full auto mode (decompile everything)"
echo "8) Exit"
echo ""

read -p "Enter choice [1-8]: " choice

case $choice in
    1)
        echo ""
        echo "Running validation..."
        python3 scripts/ghidra_sync_symbols.py --validate
        ;;
    2)
        echo ""
        read -p "Preview changes first? (y/n): " preview
        if [ "$preview" = "y" ]; then
            python3 scripts/ghidra_sync_symbols.py --export --dry-run
            echo ""
            read -p "Proceed with export? (y/n): " confirm
            if [ "$confirm" = "y" ]; then
                python3 scripts/ghidra_sync_symbols.py --export
            fi
        else
            python3 scripts/ghidra_sync_symbols.py --export
        fi
        ;;
    3)
        echo ""
        read -p "Filter by pattern (leave empty for all): " pattern
        if [ -n "$pattern" ]; then
            python3 scripts/ghidra_sync_symbols.py --import --filter "$pattern"
        else
            python3 scripts/ghidra_sync_symbols.py --import
        fi
        ;;
    4)
        echo ""
        read -p "Enter function name or address: " func
        read -p "Verify build after decompilation? (y/n): " verify
        
        if [ "$verify" = "y" ]; then
            python3 scripts/auto_decompile_function.py "$func" --verify
        else
            python3 scripts/auto_decompile_function.py "$func"
        fi
        ;;
    5)
        echo ""
        read -p "Enter pattern (e.g., 'EntityType*' or 'GetLevel*'): " pattern
        read -p "Limit number of functions (0 for no limit): " limit
        read -p "Verify each build? (y/n): " verify
        
        args="--pattern '$pattern'"
        [ "$limit" != "0" ] && args="$args --limit $limit"
        [ "$verify" = "y" ] && args="$args --verify"
        
        echo ""
        echo "Running: python3 scripts/auto_decompile.py $args"
        eval python3 scripts/auto_decompile.py $args
        ;;
    6)
        echo ""
        python3 scripts/auto_decompile.py --report
        ;;
    7)
        echo ""
        echo "⚠️  WARNING: Full auto mode will attempt to decompile ALL functions"
        read -p "Continue? (y/n): " confirm
        
        if [ "$confirm" = "y" ]; then
            read -p "Verify each build? (y/n): " verify
            read -p "Skip already attempted functions? (y/n): " skip
            
            args="--full-auto"
            [ "$verify" = "y" ] && args="$args --verify"
            [ "$skip" = "y" ] && args="$args --skip-attempted"
            
            echo ""
            echo "Running: python3 scripts/auto_decompile.py $args"
            python3 scripts/auto_decompile.py $args
        fi
        ;;
    8)
        echo "Exiting..."
        exit 0
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "=========================================="
echo "Done!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  - Review changes with: git status"
echo "  - Check build: make check"
echo "  - View progress: python3 scripts/auto_decompile.py --report"
echo "  - Read docs: cat scripts/README_DECOMPILATION.md"
echo ""
