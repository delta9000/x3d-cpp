# cmake/x3d/compiler-flags.cmake — warning policy, -Werror, and the sanitizer / fuzz instrumentation switches.

# Warning flags are directory-scoped. Vendored targets suppress their own warnings.
# cl.exe rejects GCC/Clang flags with D8021; clang-cl accepts them.
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

# The san preset enables ASan/UBSan; fuzz additionally supplies parser coverage.
option(X3D_CPP_SAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer for every project target" OFF)
if(X3D_CPP_SAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    # Line tables retain useful sanitizer stacks without full debugger records.
    add_compile_options(-g1)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_SAN=ON -> ASan + UBSan enabled (-fno-sanitize-recover=all, -g1)")
endif()
option(X3D_CPP_FUZZ "Build the libFuzzer harness for parseDocument (requires Clang)" OFF)
if(X3D_CPP_FUZZ)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "X3D_CPP_FUZZ requires Clang (libFuzzer driver). Detected: ${CMAKE_CXX_COMPILER_ID}")
    endif()
    # Parsing and node reflection live in compiled libraries. Instrument every
    # target built for the fuzz graph with coverage and sanitizers, but keep the
    # libFuzzer main out of the libraries; x3d_parse_fuzz owns the driver below.
    add_compile_options(
        -fsanitize=fuzzer-no-link,address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_FUZZ=ON -> compiled runtime layers use SanitizerCoverage + ASan + UBSan")
endif()
