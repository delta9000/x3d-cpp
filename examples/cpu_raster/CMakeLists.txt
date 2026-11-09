# Headless CPU rasterizer consuming the runtime extraction API.

cmake_minimum_required(VERSION 3.21)

# The CPU rasterizer CLI.
# PNG/JPEG textures require the stb decoder backend (ADR-0024).
if(NOT TARGET x3d_stb)
    message(FATAL_ERROR
        "cpu_raster needs the stb decode-seam backend for PNG/JPEG textures. "
        "Re-run with -DX3D_CPP_BUILD_STB=ON (or use `mise run cpuraster`).")
endif()

# PNG output: the consumer-side stb_image_write encoder, shared in third_party (not
# an SDK seam — output/encoding is the consumer's concern). Compiled in its one
# TU; warnings silenced (stb is noisy). PPM output stays dependency-free.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cpuraster/ImageWrite.cpp"
    PROPERTIES COMPILE_OPTIONS "-w")
add_executable(x3d_cpu_raster
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpuraster/ImageWrite.cpp")
target_link_libraries(x3d_cpu_raster PRIVATE x3d_cpp::x3d_cpp x3d_stb)
if(TARGET x3d_curl)
    target_link_libraries(x3d_cpu_raster PRIVATE x3d_curl)
    target_compile_definitions(x3d_cpu_raster PRIVATE X3D_CPURASTER_CURL=1)
endif()
target_include_directories(x3d_cpu_raster PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party")
target_compile_features(x3d_cpu_raster PRIVATE cxx_std_20)
target_compile_definitions(x3d_cpu_raster PRIVATE
    "X3D_CPURASTER_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
    "X3D_CPURASTER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\"")

message(STATUS "x3d_cpp: CPU rasterizer target x3d_cpu_raster configured "
               "(dependency-free, headless)")

# CPU tests.
set(CPURASTER_TESTS
    glsl_test
    rasterizer_test
    clip_plane_test
    material_shader_test
    multi_texture_test
    cube_map_test
    generated_cube_map_test
    texture_projector_test
    fog_shader_test
    glsl_interp_test
    render_smoke_test
    text_render_test
    texture_render_test
    texcoord_gen_test
    interchange_test
    linepoint_style_test
    positional_light_test
    light_ambient_test
    tier1_features_test
    skybox_test
    demo_animation_test
    author_shader_test)

add_custom_target(x3d_cpuraster_tests)
foreach(t IN LISTS CPURASTER_TESTS)
    add_executable(x3d_cpuraster_${t} "${CMAKE_CURRENT_SOURCE_DIR}/tests/${t}.cpp")
    target_link_libraries(x3d_cpuraster_${t} PRIVATE x3d_cpp::x3d_cpp x3d_stb)
    target_include_directories(x3d_cpuraster_${t} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(x3d_cpuraster_${t} PRIVATE cxx_std_20)
    target_compile_definitions(x3d_cpuraster_${t} PRIVATE
        "X3D_CPURASTER_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
        "X3D_CPURASTER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
        "X3D_CPURASTER_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests\"")
    add_dependencies(x3d_cpuraster_tests x3d_cpuraster_${t})
    add_test(NAME x3d_cpuraster_${t} COMMAND x3d_cpuraster_${t})
endforeach()
