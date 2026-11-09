# cmake/x3d/dev-tooling.cmake — compiler toolchain helpers for developers of this repo: ccache, fast linker, Ninja job pool.

# Cache launchers and global job pools affect a parent project, so developer
# tooling defaults off for add_subdirectory() consumers. Warning and sanitizer
# flags are directory-scoped. Contract: tests/cmake/dev_tooling/.
if(PROJECT_IS_TOP_LEVEL)
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" ON)
else()
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" OFF)
endif()

# Compiler launchers are cached across the consumer's build tree.
if(X3D_CPP_ENABLE_DEV_TOOLING)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
        message(STATUS "x3d_cpp: ccache enabled (${CCACHE_PROGRAM})")
    endif()
endif()

# Linker selection is directory-scoped but remains opt-in for consumers.
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

# Ninja compile pools are global and also constrain a consumer's targets.
# Defaults to logical core count; lower X3D_CPP_COMPILE_JOBS on low-memory hosts.
if(X3D_CPP_ENABLE_DEV_TOOLING)
    cmake_host_system_information(RESULT _x3d_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
    set(X3D_CPP_COMPILE_JOBS "${_x3d_logical_cores}" CACHE STRING "Max concurrent compile jobs (Ninja job pool)")
    if(CMAKE_GENERATOR MATCHES "Ninja")
        set_property(GLOBAL PROPERTY JOB_POOLS x3d_compile=${X3D_CPP_COMPILE_JOBS})
        set(CMAKE_JOB_POOL_COMPILE x3d_compile)
        message(STATUS "x3d_cpp: Ninja compile job pool = ${X3D_CPP_COMPILE_JOBS}")
    endif()
endif()
