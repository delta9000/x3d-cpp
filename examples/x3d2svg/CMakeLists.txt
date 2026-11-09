# Headless SVG projector using the public x3d_cpp::sdk facade.

cmake_minimum_required(VERSION 3.21)

add_executable(x3d_x3d2svg "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(x3d_x3d2svg PRIVATE x3d_cpp::sdk)
target_include_directories(x3d_x3d2svg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(x3d_x3d2svg PRIVATE cxx_std_20)

message(STATUS "x3d_cpp: SVG projector target x3d_x3d2svg configured "
               "(façade-only, dependency-free, headless)")

# Unit test for the projection core — pure CPU, no display. Runs in the default
# pipeline once the target is ON (cheap and deterministic).
add_executable(x3d_x3d2svg_project "${CMAKE_CURRENT_SOURCE_DIR}/tests/project_test.cpp")
target_link_libraries(x3d_x3d2svg_project PRIVATE x3d_cpp::sdk)
target_include_directories(x3d_x3d2svg_project PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(x3d_x3d2svg_project PRIVATE cxx_std_20)
target_compile_definitions(x3d_x3d2svg_project PRIVATE
    "X3D2SVG_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\"")
add_test(NAME x3d_x3d2svg_project COMMAND x3d_x3d2svg_project)

# End-to-end headless smoke: the shipped binary must extract + project the
# bundled scene and exit 0 (non-zero if zero facets) — the same --headless
# contract scripts/validate-examples.sh drives in CI.
add_test(NAME x3d_x3d2svg_smoke
    COMMAND x3d_x3d2svg "${CMAKE_CURRENT_SOURCE_DIR}/assets/smoke.x3d" --headless)
