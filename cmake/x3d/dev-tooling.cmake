# cmake/x3d/dev-tooling.cmake — compiler toolchain helpers for developers of this repo: ccache, fast linker, Ninja job pool.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Developer tooling. This is for people building THIS project, and some of it
# escapes our own subtree: the ccache launcher is written to the SHARED CACHE
# (so it becomes the default for every target in the consumer's build tree) and
# the Ninja job pool is a GLOBAL property (so it caps the consumer's compiles
# too). A consumer that merely add_subdirectory()s us must not inherit either.
# Default ON when we are the top-level project, OFF when we are somebody's
# dependency. Contract: tests/cmake/dev_tooling/.
#
# Deliberately NOT gated here: -Wall -Wextra and the sanitizer options. Those are
# directory-scoped (current dir and below), so they never reach a consumer's
# targets -- measured, not assumed. Gating them would only disable OUR OWN
# warning coverage in a vendored build, which is a regression, not a courtesy.
# X3D_CPP_SAN / X3D_CPP_FUZZ default OFF and are opt-in gates, so they cannot
# surprise anyone either.
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" ON)
else()
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" OFF)
endif()

# ---------------------------------------------------------------------------
# ccache: reuse cached object output across rebuilds. The generated headers
# change rarely, so the ~800-TU compile-test suite is almost entirely cache
# hits after the first run. Auto-detected; a no-op when ccache is absent (e.g.
# a CI runner without it), so it never breaks a fresh checkout.
# ---------------------------------------------------------------------------
# Gated: these set() calls write the SHARED cache, so as a subproject they would
# make ccache the default launcher for every target the consumer defines too.
if(X3D_CPP_ENABLE_DEV_TOOLING)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
        message(STATUS "x3d_cpp: ccache enabled (${CCACHE_PROGRAM})")
    endif()
endif()

# ---------------------------------------------------------------------------
# Fast linker: the ~70 test executables + the all-headers exe link far quicker
# with a parallel linker. Prefer mold, then lld; fall back to the system default
# (no-op) when neither is installed, so a fresh checkout / CI is never broken.
# Applies to every target defined after this point in THIS directory and below
# (add_link_options is directory-scoped; it does not reach a parent project).
# Gated anyway: a dependency silently choosing the linker for its own targets is
# still a surprise, and it is dev tooling by any reading.
# ---------------------------------------------------------------------------
if(X3D_CPP_ENABLE_DEV_TOOLING)
    find_program(X3D_MOLD mold)
    find_program(X3D_LLD ld.lld)
    if(X3D_MOLD)
        add_link_options("-fuse-ld=mold")
        message(STATUS "x3d_cpp: linker = mold")
    elseif(X3D_LLD)
        add_link_options("-fuse-ld=lld")
        message(STATUS "x3d_cpp: linker = lld")
    endif()
endif()

# ---------------------------------------------------------------------------
# Bound parallel COMPILE jobs under Ninja (a job pool; links and light syntax
# TUs stay unbounded). Ninja-only (the pool property is ignored by the Make
# generator, which we don't use for the dev preset).
#
# History: pre-C1 the inline reflection thunks made each generated TU a memory
# hog that OOM-killed cc1plus above ~-j4, so the cap was pinned at 4. After the
# C1 decl/def split (2026-06-16) the heavy fields()/thunk instantiation moved
# into the x3d_cpp_nodes library and per-compile peak RSS dropped to ~0.86 GB
# (measured), so RAM is no longer the binding constraint — the cap now defaults
# to the logical core count. Lower it (-DX3D_CPP_COMPILE_JOBS=N) only on a
# low-RAM host (budget ~0.9 GB per concurrent compile).
# ---------------------------------------------------------------------------
# Gated: JOB_POOLS is a GLOBAL property, so as a subproject this would cap the
# consumer's own compile parallelism, not just ours.
if(X3D_CPP_ENABLE_DEV_TOOLING)
    cmake_host_system_information(RESULT _x3d_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
    set(X3D_CPP_COMPILE_JOBS "${_x3d_logical_cores}" CACHE STRING "Max concurrent compile jobs (Ninja job pool)")
    if(CMAKE_GENERATOR MATCHES "Ninja")
        set_property(GLOBAL PROPERTY JOB_POOLS x3d_compile=${X3D_CPP_COMPILE_JOBS})
        set(CMAKE_JOB_POOL_COMPILE x3d_compile)
        message(STATUS "x3d_cpp: Ninja compile job pool = ${X3D_CPP_COMPILE_JOBS}")
    endif()
endif()
