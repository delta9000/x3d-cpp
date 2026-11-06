# cmake/x3d/fuzz.cmake — the libFuzzer parseDocument harness.

# Parser exceptions are allowed; sanitizer failures and crashes fail the run.
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
