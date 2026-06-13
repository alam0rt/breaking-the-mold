---
description: 'Ghidra batch documentation pass for Skullmonkeys (SLES-01090). Auto-enables all Ghidra MCP tools needed by the re-document skill.'
tools:
  # --- program / scripting ---
  - mcp_ghidra_get_current_program_info
  - mcp_ghidra_list_open_programs
  - mcp_ghidra_run_script_inline
  - mcp_ghidra_check_tools
  - mcp_ghidra_list_tool_groups
  # --- function CRUD / tags ---
  - mcp_ghidra_decompile_function
  - mcp_ghidra_batch_decompile
  - mcp_ghidra_force_decompile
  - mcp_ghidra_disassemble_function
  - mcp_ghidra_get_function_tags
  - mcp_ghidra_list_function_tags
  - mcp_ghidra_create_function_tag
  - mcp_ghidra_add_function_tag
  - mcp_ghidra_batch_add_function_tags
  - mcp_ghidra_batch_remove_function_tags
  - mcp_ghidra_search_functions_by_tag
  - mcp_ghidra_rename_function
  - mcp_ghidra_rename_function_by_address
  - mcp_ghidra_set_function_prototype
  - mcp_ghidra_batch_rename_function_components
  - mcp_ghidra_get_function_by_address
  - mcp_ghidra_get_function_variables
  - mcp_ghidra_rename_variable
  - mcp_ghidra_rename_variables
  - mcp_ghidra_set_parameter_type
  - mcp_ghidra_set_local_variable_type
  - mcp_ghidra_set_variables
  # --- comments ---
  - mcp_ghidra_get_plate_comment
  - mcp_ghidra_set_plate_comment
  - mcp_ghidra_set_decompiler_comment
  - mcp_ghidra_set_disassembly_comment
  - mcp_ghidra_batch_set_comments
  - mcp_ghidra_clear_function_comments
  # --- analysis / completeness ---
  - mcp_ghidra_analyze_function_completeness
  - mcp_ghidra_analyze_function_complete
  - mcp_ghidra_analyze_for_documentation
  - mcp_ghidra_analyze_control_flow
  - mcp_ghidra_analyze_dataflow
  - mcp_ghidra_analyze_data_region
  - mcp_ghidra_batch_analyze_completeness
  - mcp_ghidra_search_functions_enhanced
  - mcp_ghidra_get_function_pcode
  - mcp_ghidra_inspect_memory_content
  # --- xrefs / call graph ---
  - mcp_ghidra_get_function_callers
  - mcp_ghidra_get_function_callees
  - mcp_ghidra_get_function_xrefs
  - mcp_ghidra_get_xrefs_to
  - mcp_ghidra_get_xrefs_from
  - mcp_ghidra_get_bulk_xrefs
  - mcp_ghidra_analyze_call_graph
  - mcp_ghidra_get_function_call_graph
  # --- listing / enumeration ---
  - mcp_ghidra_list_functions
  - mcp_ghidra_list_functions_enhanced
  - mcp_ghidra_search_functions
  - mcp_ghidra_list_globals
  - mcp_ghidra_list_data_items
  - mcp_ghidra_list_data_items_by_xrefs
  - mcp_ghidra_list_strings
  - mcp_ghidra_search_strings
  - mcp_ghidra_get_function_count
  - mcp_ghidra_list_segments
  - mcp_ghidra_convert_number
  # --- symbols / labels ---
  - mcp_ghidra_rename_or_label
  - mcp_ghidra_create_label
  - mcp_ghidra_delete_label
  - mcp_ghidra_rename_label
  - mcp_ghidra_batch_create_labels
  - mcp_ghidra_batch_delete_labels
  - mcp_ghidra_rename_data
  - mcp_ghidra_rename_global_variable
  - mcp_ghidra_can_rename_at_address
  - mcp_ghidra_get_function_labels
  # --- data types / globals ---
  - mcp_ghidra_audit_global
  - mcp_ghidra_audit_globals_in_function
  - mcp_ghidra_set_global
  - mcp_ghidra_apply_data_type
  - mcp_ghidra_apply_data_classification
  - mcp_ghidra_analyze_struct_field_usage
  - mcp_ghidra_get_struct_layout
  - mcp_ghidra_list_data_types
  - mcp_ghidra_search_data_types
  - mcp_ghidra_get_valid_data_types
  - mcp_ghidra_validate_data_type
  - mcp_ghidra_validate_data_type_exists
  - mcp_ghidra_validate_function_prototype
  - mcp_ghidra_create_struct
  - mcp_ghidra_add_struct_field
  - mcp_ghidra_modify_struct_field
  - mcp_ghidra_remove_struct_field
  - mcp_ghidra_create_enum
  - mcp_ghidra_create_typedef
  - mcp_ghidra_create_pointer_type
  - mcp_ghidra_create_array_type
  - mcp_ghidra_clone_data_type
  - mcp_ghidra_get_type_size
  - mcp_ghidra_get_enum_values
  - mcp_ghidra_suggest_field_names
  # --- documentation archive ---
  - mcp_ghidra_apply_function_documentation
  - mcp_ghidra_get_function_documentation
  - mcp_ghidra_get_function_signature
  - mcp_ghidra_get_function_hash
  - mcp_ghidra_get_bulk_function_hashes
  - mcp_ghidra_find_undocumented_by_string
  - mcp_ghidra_batch_string_anchor_report
  - mcp_ghidra_find_similar_functions
  - mcp_ghidra_find_similar_functions_fuzzy
  - mcp_ghidra_diff_functions
  - mcp_ghidra_archive_ingest_function
  # --- read-only / general agent tools ---
  - read_file
  - file_search
  - grep_search
  - semantic_search
  - list_dir
  - manage_todo_list
  - replace_string_in_file
  - multi_replace_string_in_file
---

# Skullmonkeys Ghidra Batch Documentation

You are running the `re-document` workflow from `.claude/commands/re-document.md` (also mirrored at `.agent/skills/re-document.md`).

**Always start by reading the full skill file** ([.claude/commands/re-document.md](.claude/commands/re-document.md)) before doing anything else, then follow it to the letter.

Key behavior reminders:

- Work only on addresses in `0x80010000–0x800AFFFF` (game range).
- This is a clean-room RE project: do not trust existing names/comments without re-deriving from the decompilation and xrefs.
- Prefer `run_script_inline` over chains of read-only MCP calls — one Java script replaces 5–10 individual tool calls.
- If the sampled target belongs to a large near-identical family (e.g. `EntityDestroyCallback_Vt*`, `EntityType###_*_Init`, `PlayerCallback_*`), switch to **Opportunistic bulk documentation** (end of the skill file) and document the whole family in one transaction. Report `[BULK]` instead of `[DONE]`.
- When the user asks for a "large batch", keep looping (sample → document → recount) until either:
  - `remaining == 0`, or
  - you've completed a full bulk-family pass, or
  - you hit the context budget and need to stop.
- Print one `[DONE]` or `[BULK]` progress line per documented unit. End with a short summary.

Context files worth keeping in mind:

- [include/Game/entity.h](include/Game/entity.h), [include/Game/game_state.h](include/Game/game_state.h), [include/globals.h](include/globals.h)
- [docs/analysis/callback-install-map.md](docs/analysis/callback-install-map.md)
- [docs/reference/functions-complete.md](docs/reference/functions-complete.md)
- [docs/systems/](docs/systems/) for naming context
