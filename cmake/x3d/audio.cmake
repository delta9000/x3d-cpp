# cmake/x3d/audio.cmake — the miniaudio AudioBackend backend + swap-test.

# Optional miniaudio AudioBackend (ADR-0026); compile its implementation once.
option(X3D_CPP_BUILD_MINIAUDIO "Build miniaudio AudioBackend backend (OFF default; fetches miniaudio v0.11.25)" OFF)

if(X3D_CPP_BUILD_MINIAUDIO)
    include(FetchContent)
    # miniaudio is a single-header library; fetch only the source tree and use
    # its repo root as a PRIVATE include directory. Do not add_subdirectory its
    # own CMake project — x3d_miniaudio only needs miniaudio.h.
    FetchContent_Declare(miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE)
    FetchContent_GetProperties(miniaudio)
    if(NOT miniaudio_POPULATED)
        # Single-arg FetchContent_Populate is deprecated under CMP0169 (CMake
        # 3.30+); set the policy to OLD to keep using it. The policy is unknown
        # to CMake < 3.30 (e.g. 3.28 on CI), where setting it is a hard error —
        # so guard the SET on POLICY existence. PUSH/POP scope it either way.
        cmake_policy(PUSH)
        if(POLICY CMP0169)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Populate(miniaudio)
        cmake_policy(POP)
    endif()

    # Node-graph spinlocks need pthread symbols even with MA_NO_DEVICE_IO.
    find_package(Threads REQUIRED)

    add_library(x3d_miniaudio STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio/MiniaudioBackend.cpp")
    target_link_libraries(x3d_miniaudio PUBLIC x3d_cpp::x3d_cpp)
    # Link Threads publicly so any consumer of this lib inherits the link.
    target_link_libraries(x3d_miniaudio PUBLIC Threads::Threads)
    target_include_directories(x3d_miniaudio PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio")
    # miniaudio.h is PRIVATE: only MiniaudioBackend.cpp needs it; consumers
    # that link x3d_miniaudio inherit no miniaudio headers.
    target_include_directories(x3d_miniaudio PRIVATE
        "${miniaudio_SOURCE_DIR}")
    # miniaudio is warning-noisy on the TU that defines MINIAUDIO_IMPLEMENTATION.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_miniaudio PRIVATE -w)  # fetched single header.
    endif()

    # Compare §16 synthesis and spatial behavior without opening an audio device.
    if(X3D_CPP_BUILD_TESTS)
        add_executable(x3d_sound_swaptest
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/dsp/BuiltinDspBackend.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/tests/sound_swap_test.cpp")
        target_link_libraries(x3d_sound_swaptest PRIVATE x3d_cpp::x3d_cpp x3d_miniaudio)
        add_test(NAME x3d_sound_swaptest COMMAND x3d_sound_swaptest)
        set_tests_properties(x3d_sound_swaptest PROPERTIES TIMEOUT 120)
    endif()
endif()
