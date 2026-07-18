# examples/cpu_raster/CMakeLists.txt
#
# Out-of-SDK HEADLESS CPU RASTERIZER (a CONSUMER of the x3d_cpp extraction seam,
# NOT part of the SDK). Firewalled behind X3D_CPP_BUILD_CPURASTER=OFF at the root
# so the default build/golden/ctest path never configures or compiles any of it.
#
# Unlike the PoC OpenGL renderer this target has ZERO third-party dependencies —
# no GLFW, no glad, no OpenGL, no image library. It rasterizes on the CPU and
# writes a PPM, so it builds and RUNS on a bare CI runner with no display. That
# makes both the binary AND its ctest suite safe to run in the default pipeline.
#
# Entered only via the root CMake's
#   if(X3D_CPP_BUILD_CPURASTER) add_subdirectory(examples/cpu_raster) endif()
# so it can assume the x3d_cpp::x3d_cpp INTERFACE target already exists.

cmake_minimum_required(VERSION 3.21)

# --------------------------------------------------------------------------
# The CPU rasterizer CLI.
# --------------------------------------------------------------------------
# PNG/JPEG textures are decoded through the SDK's first-party TextureResolver
# decode-seam backend (x3d_stb, ADR-0024), which the consumer EXPLICITLY opts
# into with -DX3D_CPP_BUILD_STB=ON — proving the seam composes from outside the
# SDK rather than the example vendoring a parallel decoder. (proc:/.ppm stay
# built-in.) `mise run cpuraster` passes the flag; fail loudly if it is missing.
if(NOT TARGET x3d_stb)
    message(FATAL_ERROR
        "cpu_raster needs the stb decode-seam backend for PNG/JPEG textures. "
        "Re-run with -DX3D_CPP_BUILD_STB=ON (or use `mise run cpuraster`).")
endif()

# PNG output: the consumer-side stb_image_write encoder, vendored locally (NOT
# an SDK seam — output/encoding is the consumer's concern). Compiled in its one
# TU; warnings silenced (stb is noisy). PPM output stays dependency-free.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cpuraster/ImageWrite.cpp"
    PROPERTIES COMPILE_OPTIONS "-w")
add_executable(x3d_cpu_raster
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpuraster/ImageWrite.cpp")
target_link_libraries(x3d_cpu_raster PRIVATE x3d_cpp::x3d_cpp x3d_stb)
target_include_directories(x3d_cpu_raster PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_compile_features(x3d_cpu_raster PRIVATE cxx_std_20)
target_compile_definitions(x3d_cpu_raster PRIVATE
    "X3D_CPURASTER_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
    "X3D_CPURASTER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\"")

message(STATUS "x3d_cpp: CPU rasterizer target x3d_cpu_raster configured "
               "(dependency-free, headless)")

# --------------------------------------------------------------------------
# Unit tests — all pure CPU, no display, registered into ctest. These DO run in
# the default pipeline (the target is opt-in, but once ON the tests are cheap and
# deterministic).
# --------------------------------------------------------------------------
set(CPURASTER_TESTS
    glsl_test
    rasterizer_test
    material_shader_test
    glsl_interp_test
    render_smoke_test
    text_render_test
    texture_render_test
    positional_light_test
    tier1_features_test
    skybox_test
    demo_animation_test)

foreach(t IN LISTS CPURASTER_TESTS)
    add_executable(x3d_cpuraster_${t} "${CMAKE_CURRENT_SOURCE_DIR}/tests/${t}.cpp")
    target_link_libraries(x3d_cpuraster_${t} PRIVATE x3d_cpp::x3d_cpp x3d_stb)
    target_include_directories(x3d_cpuraster_${t} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(x3d_cpuraster_${t} PRIVATE cxx_std_20)
    target_compile_definitions(x3d_cpuraster_${t} PRIVATE
        "X3D_CPURASTER_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
        "X3D_CPURASTER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
        "X3D_CPURASTER_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests\"")
    add_test(NAME x3d_cpuraster_${t} COMMAND x3d_cpuraster_${t})
endforeach()
