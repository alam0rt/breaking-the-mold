# =============================================================================
# port/CMakeLists.txt  --  native 32-bit PC build of the Skullmonkeys decomp
# =============================================================================
# This is ADDITIVE and invisible to the MIPS byte-match build (`make check`).
# It compiles the shared game C (../src/*.c) with the TARGET_PC pivot injected
# via `-include port_config.h`, links it against the SPEC/HAL layer (port/spec)
# and the generated weak stubs/globals (port/decomp), and produces `skullmonkeys`.
#
# Requires a 32-bit toolchain (-m32) + 32-bit libc/libgcc, and (optionally for a
# real window) SDL2 + OpenGL. See ../flake.nix and ../docs/plans/pc-port.md §7.
#
# Configure/build (Ninja is the default generator via CMakePresets.json):
#     cmake --preset linux-debug
#     cmake --build --preset linux-debug
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(skullmonkeys_port C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)   # decomp C is not strict C11
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# --- 32-bit ABI: load-bearing, not a preference (see plan §2) ----------------
add_compile_options(-m32)
add_link_options(-m32)

# --- regenerate the weak stub / global TUs at configure time -----------------
# Keeps _autostubs.c / _autoglobals.c in sync with asm/ + src/ without a manual
# step. (A custom target could re-run on every build; configure-time is enough
# for Phase 0 and avoids a hard Python dependency in the build graph.)
find_program(PYTHON3 python3)
if(PYTHON3)
    execute_process(
        COMMAND ${PYTHON3} ${REPO_ROOT}/tools/gen_port_stubs.py
        WORKING_DIRECTORY ${REPO_ROOT}
        RESULT_VARIABLE _gen_rc)
    if(NOT _gen_rc EQUAL 0)
        message(WARNING "gen_port_stubs.py failed (rc=${_gen_rc}); using existing generated files")
    endif()
    # Faithfully transcribe splat pointer-table data segments (entity/render
    # vtables) to C so the render/entity dispatch resolves to real symbols
    # rather than weak-zeroed slots. Only pure pointer-table .s files here.
    execute_process(
        COMMAND ${PYTHON3} ${REPO_ROOT}/tools/gen_port_data.py
                ${REPO_ROOT}/asm/data/gfx.rodata.s
                --out ${CMAKE_CURRENT_SOURCE_DIR}/decomp/data
        WORKING_DIRECTORY ${REPO_ROOT}
        RESULT_VARIABLE _gendata_rc)
    if(NOT _gendata_rc EQUAL 0)
        message(WARNING "gen_port_data.py failed (rc=${_gendata_rc}); using existing generated files")
    endif()
else()
    message(WARNING "python3 not found; using checked-in port/decomp/_auto*.c as-is")
endif()

# --- the TARGET_PC pivot: injected into EVERY translation unit ---------------
# Order matters: port_config.h must be seen before common.h so its INCLUDE_ASM /
# PSX_REG neutralisation and the 32-bit assert take effect. port/include comes
# first on the include path so LIB*.H resolve to the PC shims, not psyq/.
add_compile_definitions(TARGET_PC=1 _LANGUAGE_C)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include   # port shims (LIB*.H -> psyq_pc.h) FIRST
    ${REPO_ROOT}/include                  # shared game headers (common.h, Game/)
)
add_compile_options(-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_config.h)

# --- tolerate the decomp's pre-C99-strict idioms on a modern GCC -------------
# GCC 14+ promotes these to hard errors; the byte-match C relies on the lax
# behaviour. None of these affect codegen correctness for the port.
add_compile_options(
    -fno-strict-aliasing
    -fno-builtin              # game supplies its own mem*/str* in some paths
    -fcommon                  # tentative defs merge (decomp targets gcc 2.7.2;
                              # modern gcc defaults to -fno-common, which turns
                              # the engine's multi-TU `T x asm("D_..")` gp-rel
                              # tentative globals into duplicate strong defs)
    -Wno-incompatible-pointer-types
    -Wno-int-conversion
    -Wno-implicit-function-declaration
    -Wno-builtin-declaration-mismatch
    -Wno-implicit-int
)

# --- sources -----------------------------------------------------------------
file(GLOB GAME_SRC       CONFIGURE_DEPENDS ${REPO_ROOT}/src/*.c)
file(GLOB SPEC_SRC       CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/spec/*.c)
# decomp/ holds the generated weak stub/global TUs at the top level AND the
# hand/m2c functional-C bodies for still-asm GAME functions under per-segment
# subdirs (decomp/<seg>/<Func>.c). Recurse so both are compiled; a strong body
# there overrides the weak stub in _autostubs.c automatically.
file(GLOB_RECURSE DECOMP_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/decomp/*.c)

# NOTE: src/libs/*.c (the decompiled PSY-Q libraries: libcd, libspu, libvoice)
# are DELIBERATELY EXCLUDED. They are matched MIPS-faithful C that dereferences
# PSX kernel/hardware globals (BIOS_CALLBACK_TABLES, SPU_REGISTER_BASE, ...)
# which don't exist on PC. Per docs/plans/pc-port.md §3, the HAL layer REPLACES
# them: the full PSY-Q surface (Cd*, Spu*, Pad*, VSync*, *Callback, ...) is
# provided by port/spec/{cd_files,spu_sdl,pad_sdl,gte,gpu_gl,bios}.c instead.

add_executable(skullmonkeys
    ${GAME_SRC}
    ${SPEC_SRC}
    ${DECOMP_SRC}
)

# --- optional SDL2 + OpenGL --------------------------------------------------
# host_main.c compiles with or without SDL (falls back to a windowless boot that
# still exercises the link + HAL abort path). When SDL2/GL are present we link
# them and flag the SDL path on. 32-bit packages required (pkgsi686Linux.*).
#
# The 32-bit SDL2 lives at a pkg-config prefix the flake exports as
# PORT_PKG_CONFIG_PATH (see flake.nix shellHook). Point pkg-config there so we
# resolve the i686 build rather than any 64-bit SDL2 on the default path.
find_package(PkgConfig QUIET)
set(SDL2_FOUND FALSE)
if(PkgConfig_FOUND)
    if(DEFINED ENV{PORT_PKG_CONFIG_PATH})
        set(ENV{PKG_CONFIG_PATH} "$ENV{PORT_PKG_CONFIG_PATH}")
    endif()
    pkg_check_modules(SDL2 QUIET sdl2)
endif()

if(SDL2_FOUND)
    target_compile_definitions(skullmonkeys PRIVATE PORT_HAVE_SDL2=1)
    target_include_directories(skullmonkeys PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(skullmonkeys PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(skullmonkeys PRIVATE ${SDL2_LIBRARIES})
    message(STATUS "SDL2 found (pkg-config): real window enabled")
else()
    message(STATUS "SDL2 not found: building windowless (link + HAL-abort probe only)")
endif()

# OpenGL (32-bit libGL). host GL is loaded via SDL_GL_GetProcAddress at runtime,
# so an explicit link is optional; add it when the loader library is available.
find_package(OpenGL QUIET)
if(OpenGL_OpenGL_FOUND OR OPENGL_FOUND)
    target_link_libraries(skullmonkeys PRIVATE ${OPENGL_LIBRARIES})
endif()

# math + dl are cheap and often needed by the decomp/libc glue.
target_link_libraries(skullmonkeys PRIVATE m)

message(STATUS "Port sources: ${CMAKE_CURRENT_SOURCE_DIR} (32-bit, TARGET_PC)")
