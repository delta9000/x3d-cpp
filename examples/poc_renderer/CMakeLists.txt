# Optional OpenGL renderer consuming the runtime extraction API.

cmake_minimum_required(VERSION 3.21)

# GLFW via FetchContent. Pinned to a >=3.4 release tag (3.4 is the first GLFW
# with stable native-Wayland-by-default support). Configured so only the
# library builds.
include(FetchContent)

set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE) # native Wayland (primary target)
set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE) # XWayland safety net
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4               # >=3.4 required for native Wayland default
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)
# Suppress warnings from third-party GLFW sources.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(glfw PRIVATE -w)
endif()

# Dear ImGui overlay. ImGui ships no top-level CMake target, so build the core
# sources plus the GLFW/OpenGL3 backends as a tiny static lib local to the PoC.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# The committed glad loader is C; enable its toolchain in this optional subdirectory.
enable_language(C)
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/glad")
add_library(poc_glad STATIC "${GLAD_DIR}/src/glad.c")
target_include_directories(poc_glad PUBLIC "${GLAD_DIR}/include")
# glad needs dlopen for gladLoadGL(glfwGetProcAddress) at runtime.
target_link_libraries(poc_glad PUBLIC ${CMAKE_DL_LIBS})

add_library(poc_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
target_include_directories(poc_imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends")
target_link_libraries(poc_imgui PUBLIC glfw poc_glad)
# ImGui uses its bundled GL loader.
target_compile_definitions(poc_imgui PUBLIC GLFW_INCLUDE_NONE)
target_compile_features(poc_imgui PUBLIC cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(poc_imgui PRIVATE -w)
endif()

find_package(Threads REQUIRED)

# Compile stb_image in its own translation unit.
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
add_library(poc_stb_image STATIC "${STB_DIR}/stb_image_impl.cpp")
target_include_directories(poc_stb_image PUBLIC "${STB_DIR}")

add_executable(x3d_poc_renderer
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp")

target_link_libraries(x3d_poc_renderer
    PRIVATE
        x3d_cpp::x3d_cpp
        x3d_stbtt          # Glyph atlas (StbttGlyphAtlas) for Text rendering
        x3d_plmpeg         # pl_mpeg decoder for MovieTexture (ADR-0041)
        glfw
        poc_glad
        poc_imgui
        poc_stb_image
        Threads::Threads
        ${CMAKE_DL_LIBS})

# OpenGL: glad provides the function pointers, but the process still links the
# GL client lib for the GLX/EGL bootstrap glfwGetProcAddress resolves against.
find_package(OpenGL)
if(OpenGL_FOUND)
    target_link_libraries(x3d_poc_renderer PRIVATE OpenGL::GL)
endif()

target_compile_features(x3d_poc_renderer PRIVATE cxx_std_20)

# The std-only DDS decoder (runtime/io/dds) the TextureResolver backends share;
# the PoC's own texture resolver decodes DDS 2D and cube-map images with it.
target_include_directories(x3d_poc_renderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/io/dds")

# Surface the asset/shaders dir to the binary so it can find the bundled
# hand-written triangle scene + GLSL without a working-directory dance.
target_compile_definitions(x3d_poc_renderer PRIVATE
    # Use glad instead of the system GL headers in every translation unit.
    GLFW_INCLUDE_NONE
    "X3D_POC_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
    "X3D_POC_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
    # Portable shaders are shared with the CPU interpreter.
    "X3D_POC_PORTABLE_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../cpu_raster/shaders\""
    # Bundled Liberation faces (shared with the SDK text tests; SIL OFL).
    "X3D_POC_FONT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/fonts\"")

message(STATUS "x3d_cpp: PoC renderer target x3d_poc_renderer configured "
               "(GLFW Wayland+X11, glad core-3.3, Dear ImGui overlay)")

# Pure input-layer unit test (no GL context; only needs the GLFW + SDK headers).
add_executable(x3d_poc_input
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/input_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp")
target_link_libraries(x3d_poc_input PRIVATE x3d_cpp::x3d_cpp glfw)
target_compile_features(x3d_poc_input PRIVATE cxx_std_20)
# No GL loader here at all (input.cpp uses only the GLFW window/input API), so keep
# GLFW from pulling in the system <GL/gl.h> — same reason as the renderer target.
target_compile_definitions(x3d_poc_input PRIVATE GLFW_INCLUDE_NONE)
add_test(NAME x3d_poc_input COMMAND x3d_poc_input)

# Pure draw-math unit test (no GL context; verifies the optimized normal matrix
# matches a full 4x4 inverse-transpose reference).
add_executable(x3d_poc_draw_math
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/draw_math_test.cpp")
target_link_libraries(x3d_poc_draw_math PRIVATE x3d_cpp::x3d_cpp)
target_compile_features(x3d_poc_draw_math PRIVATE cxx_std_20)
add_test(NAME x3d_poc_draw_math COMMAND x3d_poc_draw_math)

# Real GL regression: the posed skin must match equivalent ordinary geometry,
# and differ from the bind pose. This fails if the skin shader is disabled.
add_test(NAME x3d_poc_skin_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/skin_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer> "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Real GL regression: positional lighting must reach both fragment paths.
add_test(NAME x3d_poc_positional_light_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/positional_light_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: eye-space texture coordinate generation.
add_test(NAME x3d_poc_texgen_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/texgen_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: ClipPlane cuts every built-in program.
add_test(NAME x3d_poc_clip_plane_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/clip_plane_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: LocalFog fogs its scope in every built-in program.
add_test(NAME x3d_poc_local_fog_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/local_fog_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: shadow-enabled lights are occluded by castShadow shapes.
add_test(NAME x3d_poc_shadow_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/shadow_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: ComposedCubeMapTexture environment mapping.
add_test(NAME x3d_poc_cube_map_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/cube_map_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: GeneratedCubeMapTexture faces rendered from the scene.
add_test(NAME x3d_poc_generated_cube_map_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_cube_map_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: texture projectors light the scene through their texture.
add_test(NAME x3d_poc_texture_projector_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/texture_projector_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: local, noise and refraction texture-coordinate generators.
add_test(NAME x3d_poc_texgen_local_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/texgen_local_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: MultiTexture stages combine with their modes and UV sets.
add_test(NAME x3d_poc_multitexture_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/multitexture_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)

# Real GL regression: scene-authored shaders select, bind uniforms, fall back.
add_test(NAME x3d_poc_author_shader_gl
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tests/author_shader_gl_test.py"
            $<TARGET_FILE:x3d_poc_renderer>)
