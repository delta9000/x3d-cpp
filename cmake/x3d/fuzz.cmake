# cmake/x3d/fuzz.cmake — the libFuzzer parseDocument harness.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# BLD-2: libFuzzer harness for parseDocument (Clang-only). Builds only when
# X3D_CPP_FUZZ=ON and the compiler is Clang (libFuzzer requires the
# `-fsanitize=fuzzer` driver, which is GCC 14+ but most reliable on Clang).
# The fuzz preset sets both; the `cpp-fuzz` CI job uses the same. The
# harness itself catches all exceptions and just asserts the parser doesn't
# crash — the ASan/UBSan signal handlers are what actually catch the bug.
# ---------------------------------------------------------------------------
if(X3D_CPP_FUZZ)
    add_executable(x3d_parse_fuzz
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/parse_fuzz.cpp")
    target_link_libraries(x3d_parse_fuzz PRIVATE x3d_cpp::sdk)
    # The compiled dependency graph receives fuzzer-no-link coverage plus
    # ASan/UBSan above. This executable adds the libFuzzer driver and also
    # instruments its small adapter callback.
    target_compile_options(x3d_parse_fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    # Link the libFuzzer driver + ASan/UBSan runtimes so the fuzzer can run + signal.
    target_link_options(x3d_parse_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_FUZZ=ON -> x3d_parse_fuzz built with libFuzzer + ASan + UBSan")
endif()
