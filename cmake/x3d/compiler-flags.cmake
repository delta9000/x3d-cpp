# cmake/x3d/compiler-flags.cmake — warning policy, -Werror, and the sanitizer / fuzz instrumentation switches.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Project-wide warning policy. -Wall -Wextra applied to every target we own;
# vendored TUs (Jolt, quickjs, stb, wuffs, stb_truetype) suppress with their own
# `target_compile_options(... PRIVATE -w)` later in this file so they don't pull
# in this baseline. X3D_CPP_WERROR (default OFF) promotes warnings to errors
# for the merge gate — the `ci` CMake preset flips it ON so local `mise run ci`
# matches the GitHub Actions fast gate.
#
# The -W flags are GCC/Clang syntax. Real MSVC (cl.exe) rejects them — passing
# `-Wextra` fails configure with `D8021 invalid numeric argument`. clang-cl
# reports as Clang (not MSVC) and accepts the GCC/Clang flags, so the MSVC branch
# below fires ONLY for genuine cl.exe (the local clang-cl cross-build and the
# GitHub MSVC job take opposite branches, which is intended).
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W3)   # MSVC baseline; the tree is not yet /W4-clean
else()
    add_compile_options(-Wall -Wextra)
endif()
option(X3D_CPP_WERROR "Treat compiler warnings as errors (the merge gate enables this via the ci preset)" OFF)
if(X3D_CPP_WERROR)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
    message(STATUS "x3d_cpp: X3D_CPP_WERROR=ON -> warnings treated as errors")
endif()

# ---------------------------------------------------------------------------
# Sanitizer gates (BLD-2). X3D_CPP_SAN enables AddressSanitizer + UndefinedBehavior
# Sanitizer for every project target — the green doctest run misses the kind of
# memory/UB bugs that SEC-1/2 + MEM-3/4 in the project backlog expose. The `san`
# CMake preset flips it ON; the `cpp-san` GitHub Actions job uses the same.
# X3D_CPP_FUZZ additionally enables the libFuzzer harness for parseDocument
# (Clang-only). The two options compose — `san` is the test gate, `fuzz` is the
# explore-time harness.
# ---------------------------------------------------------------------------
option(X3D_CPP_SAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer for every project target" OFF)
if(X3D_CPP_SAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    # CI needs symbolized function/source locations, not full debugger variable
    # and type records. Debug's default -g made the behavior-only sanitizer tree
    # roughly 24 GB; the later -g1 keeps useful ASan/UBSan stacks while bounding
    # object and executable size.
    add_compile_options(-g1)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_SAN=ON -> ASan + UBSan enabled (-fno-sanitize-recover=all, -g1)")
endif()
option(X3D_CPP_FUZZ "Build the libFuzzer harness for parseDocument (requires Clang)" OFF)
if(X3D_CPP_FUZZ)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "X3D_CPP_FUZZ requires Clang (libFuzzer driver). Detected: ${CMAKE_CXX_COMPILER_ID}")
    endif()
    # Parsing and node reflection now live in compiled libraries. Instrument every
    # target built for the fuzz graph with coverage and sanitizers, but keep the
    # libFuzzer main out of the libraries; x3d_parse_fuzz owns the driver below.
    add_compile_options(
        -fsanitize=fuzzer-no-link,address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_FUZZ=ON -> compiled runtime layers use SanitizerCoverage + ASan + UBSan")
endif()
