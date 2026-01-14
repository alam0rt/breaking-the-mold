-- Luacheck configuration for PCSX-Redux Lua scripts
-- https://luacheck.readthedocs.io/

-- Ignore line length warnings (default 120, we have long comments)
max_line_length = false

-- Allow unused arguments (common in callback functions)
unused_args = false

-- PCSX-Redux global variables
globals = {
    "PCSX",          -- PCSX-Redux Lua API
    "bit",           -- LuaJIT bit operations
    "mem",           -- Memory access (PCSX.getMemPtr())
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
