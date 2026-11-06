# cmake/x3d/options.cmake — top-level build options: tests, examples, optional consumers, extensions.

# Tests: compile every generated header.
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

# Isolate header failures to one translation unit; the dev preset skips this
# slower sweep while CI retains it.
option(X3D_CPP_PER_HEADER_CHECKS "Compile each header in isolation as a ctest" ON)

# Opt-in OpenGL consumer; enabling it can fetch GLFW.
option(X3D_CPP_BUILD_POC "Build the PoC OpenGL renderer" OFF)

option(X3D_CPP_BUILD_ASSET_IMPORT "Build the asset-import consumer" OFF)

option(X3D_CPP_BUILD_CPURASTER "Build the headless CPU rasterizer + GLSL emulation" OFF)

option(X3D_CPP_BUILD_X3D2SVG "Build the headless X3D->SVG projector example" OFF)

# Extensions may include core headers; core must not include runtime/ext/.
option(X3D_CPP_BUILD_EXT "Build the opt-in x3d::runtime::ext extensions (foreign-format codecs, binary geometry)" OFF)

option(X3D_CPP_BUILD_EXAMPLES "Build + run the x3d::sdk façade examples" ${PROJECT_IS_TOP_LEVEL})
