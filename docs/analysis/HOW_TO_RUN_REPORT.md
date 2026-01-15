# How to Generate Actual Ghidra Report

## Prerequisites

1. Ghidra installed and configured
2. SLES_010.90 project open
3. Ghidra MCP server running on port 8192

## Option 1: Using Copilot MCP Integration

Simply ask Copilot:
```
"List all unknown functions from Ghidra and create a report"
```

Copilot will use the MCP tools directly:
- `mcp_ghidra_functions_list` - Get all functions
- `mcp_ghidra_xrefs_list` - Get cross-references
- `mcp_ghidra_functions_decompile` - Get source

## Option 2: Manual Script Execution

```bash
# Ensure Ghidra is running
# Then run:
python3 scripts/generate_ghidra_report.py
```

## What Gets Generated

1. `unknown_functions.txt` - Symbol entries to add to symbol_addrs.txt
2. `function_analysis_report.md` - Comprehensive analysis with priorities
3. `decompilation_queue.txt` - Ordered list for m2c processing

## Next Steps After Generation

1. Review high-priority functions in report
2. Start with Entity System and Player System categories
3. Decompile using m2c workflow
4. Document findings in evil-engine/docs
5. Update Ghidra with verified names and comments
6. Add to symbol_addrs.txt
