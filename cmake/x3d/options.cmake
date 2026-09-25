# cmake/x3d/options.cmake — top-level build options: tests, examples, optional consumers, extensions.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Tests: compile every generated header.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_TESTS "Build header-compilation tests for x3d_cpp" ${PROJECT_IS_TOP_LEVEL})

# All doctest suites use the same implementation-providing entry point. Compile
# it once per configuration instead of once in every grouped test executable.
if(X3D_CPP_BUILD_TESTS)
    add_library(x3d_doctest_main STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp")
    target_include_directories(x3d_doctest_main PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    target_compile_features(x3d_doctest_main PUBLIC cxx_std_20)
endif()

# Per-header isolation compile-tests (the ~800 compile_* ctests, one per header).
# They pin a break to a single header, but the aggregate x3d_cpp_all_headers TU
# plus the behavior/integration suite already catch breaks — the per-header sweep
# is the slow part of a cold ctest. ON by default so CI keeps full coverage; the
# dev preset sets it OFF for fast local iteration.
option(X3D_CPP_PER_HEADER_CHECKS "Compile each header in isolation as a ctest" ON)

# ---------------------------------------------------------------------------
# PoC OpenGL renderer (examples/poc_renderer/). OUT-of-SDK consumer, OFF by
# default so the normal build/golden/ctest path is completely unaffected: with
# this OFF we never add_subdirectory it, so no FetchContent of GLFW, no glad
# TU, and it never joins the OOM-prone all-headers compile. It is added at the
# very end of this file once the x3d_cpp::x3d_cpp INTERFACE target exists.
# Build it with:  cmake --preset dev -DX3D_CPP_BUILD_POC=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_POC "Build the PoC OpenGL renderer" OFF)

# Asset-import consumer + authoring footprint smoke (examples/asset_import/).
# OFF by default; Task 11 finalizes the subdirectory wiring. The authoring
# footprint smoke test is registered at the end of this file when this is ON.
option(X3D_CPP_BUILD_ASSET_IMPORT "Build the asset-import consumer" OFF)

# ---------------------------------------------------------------------------
# Headless CPU rasterizer (examples/cpu_raster/). OUT-of-SDK consumer, OFF by
# default. Unlike the PoC it has NO third-party deps (no GLFW/glad/GL/image lib),
# rasterizes on the CPU and writes a PPM, so both the binary and its ctest suite
# run on a bare CI runner. Added at the end of this file once x3d_cpp::x3d_cpp
# exists. Build with:  cmake -S . -B build-cpuraster -DX3D_CPP_BUILD_CPURASTER=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_CPURASTER "Build the headless CPU rasterizer + GLSL emulation" OFF)

# ---------------------------------------------------------------------------
# Headless SVG projector (examples/x3d2svg/). OUT-of-SDK consumer, OFF by
# default. The smallest reference consumer: no third-party deps, and it links
# ONLY the public façade x3d_cpp::sdk (not the internal target cpu_raster uses).
# Added at the end of this file once x3d_cpp::sdk exists. Build with:
#   cmake -S . -B build-x3d2svg -DX3D_CPP_BUILD_X3D2SVG=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_X3D2SVG "Build the headless X3D->SVG projector example" OFF)

# ---------------------------------------------------------------------------
# Extension quarantine (runtime/ext/). Hand-written foreign-format codecs and
# binary-geometry helpers that live in x3d::runtime::ext. One-way dep: ext
# may include runtime/extract/* (PackedMesh, Aabb, …) but core NEVER includes
# runtime/ext/*. Default OFF so the standard build/golden/ctest path is
# completely unaffected. Build with: cmake --preset dev -DX3D_CPP_BUILD_EXT=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_EXT "Build the opt-in x3d::runtime::ext extensions (foreign-format codecs, binary geometry)" OFF)

# ---------------------------------------------------------------------------
# SDK façade examples (examples/0[123]_*.cpp). Headless, std-only, link
# x3d_cpp::sdk. ON when this is the top-level project so the v1 examples build +
# run as ctests; a downstream consumer that pulls x3d_cpp in gets them OFF.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_EXAMPLES "Build + run the x3d::sdk façade examples" ${PROJECT_IS_TOP_LEVEL})
