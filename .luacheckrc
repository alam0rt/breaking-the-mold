-- Luacheck configuration for PCSX-Redux Lua scripts
-- https://luacheck.readthedocs.io/

-- Ignore line length warnings (default 120, we have long comments)
max_line_length = false

-- Allow unused arguments (common in callback functions)
unused_args = false

-- Allow variables starting with _ to be unused (common convention)
unused_secondaries = false

-- PCSX-Redux global variables
globals = {
    "PCSX",          -- PCSX-Redux Lua API
    "bit",           -- LuaJIT bit operations
    "mem",           -- Memory access (PCSX.getMemPtr())
    
    -- User-facing REPL functions (intentionally global for interactive use)
    "dump_log",      -- Save buffered log entries to disk
    "clear_log",     -- Clear the log buffer
    "mark",          -- Add trace marker
    "markers",       -- Show all markers
    "status",        -- Show current game status
    "entities",      -- List active entities
    "stats",         -- Show statistics
    "test_logging",  -- Test logging system
    "snapshot",      -- Get current state snapshot
    "levels",        -- List all levels
    "load_level",    -- Load a specific level
    "load_level_by_id", -- Load level by ID string
    "level_info",    -- Show current level info
    "set_boot_override", -- Set boot override
    "reload_watchers", -- Reload breakpoints
    "export_trace",  -- Export trace to evil-engine format
}

-- Standard Lua globals (read-only)
read_globals = {
    -- Standard library
    "assert", "error", "ipairs", "next", "pairs", "pcall", "print",
    "require", "select", "tonumber", "tostring", "type", "unpack",
    "xpcall", "_VERSION",
    
    -- Standard modules
    "coroutine", "debug", "io", "math", "os", "package", "string", "table",
    
    -- LuaJIT specific
    "jit",
}

-- Ignore specific warnings
ignore = {
    "212",  -- Unused argument (callback functions)
    "611",  -- Whitespace
}

-- Files/directories to exclude
exclude_files = {
    ".venv/**",
    "build/**",
    "tools/**/*.lua",  -- Third-party tools
}
