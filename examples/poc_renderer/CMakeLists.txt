# examples/poc_renderer/CMakeLists.txt
#
# Out-of-SDK PoC OpenGL renderer (the renderer is a CONSUMER of the headless
# x3d_cpp SDK, NOT part of it). Firewalled behind X3D_CPP_BUILD_POC=OFF at the
# root, so the default `mise run build` / golden / ctest path never configures
# or compiles any of this and never joins the OOM-prone all-headers TU.
#
# This subdir is only ever entered via the root CMake's
#   if(X3D_CPP_BUILD_POC) add_subdirectory(examples/poc_renderer) endif()
# so it can assume the x3d_cpp::x3d_cpp INTERFACE target already exists.
#
# Dependencies:
#   * GLFW (>=3.4) via FetchContent — Wayland ON (native), X11 ON as an XWayland
#     safety net. GLFW's own examples/tests/docs are OFF (we only want the lib).
#   * Dear ImGui via FetchContent — linked only into this optional PoC target.
#   * glad core-3.3 — PRE-GENERATED real sources committed at
#     ../../third_party/glad/ (NOT a generator FetchContent). Built here as a
#     tiny static lib.
#
# M0 (task T9) gate: configure + build + LINK the binary. The GUI is NOT run in
# CI / headless; the user runs it on a real Wayland session.

cmake_minimum_required(VERSION 3.20)

# --------------------------------------------------------------------------
# GLFW via FetchContent. Pinned to a >=3.4 release tag (3.4 is the first GLFW
# with stable native-Wayland-by-default support). Configured so only the
# library builds.
# --------------------------------------------------------------------------
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

# --------------------------------------------------------------------------
# Dear ImGui overlay. ImGui ships no top-level CMake target, so build the core
# sources plus the GLFW/OpenGL3 backends as a tiny static lib local to the PoC.
# --------------------------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# --------------------------------------------------------------------------
# glad core-3.3 loader — PRE-GENERATED sources committed in-repo. Built as a
# tiny static lib so the loader's translation unit is compiled exactly once.
#
# The root project declares only LANGUAGES CXX (the SDK is header-only C++), so
# the C toolchain is not configured by default. glad.c is C, hence enable it
# here — scoped to the PoC subdir, which is only entered when X3D_CPP_BUILD_POC
# is ON, so the default C++-only build is untouched.
# --------------------------------------------------------------------------
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
# ImGui's opengl3 backend uses its own bundled GL loader (imgui_impl_opengl3_loader.h)
# since 1.82 and ignores the old IMGUI_IMPL_OPENGL_LOADER_* macros — so we set none.
target_compile_definitions(poc_imgui PUBLIC GLFW_INCLUDE_NONE)
target_compile_features(poc_imgui PUBLIC cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(poc_imgui PRIVATE -w)
endif()

# --------------------------------------------------------------------------
# The PoC renderer executable. Links the INTERFACE SDK alias + glfw + glad +
# the GL/dl/pthread libs. main.cpp is the only .cpp in the whole project that
# is NOT header-only — by design, it lives OUTSIDE the SDK.
# --------------------------------------------------------------------------
find_package(Threads REQUIRED)

# stb_image (B8) — vendored public-domain image loader. The ~8k-line
# implementation compiles in its OWN TU (stb_image_impl.cpp) so main.cpp only
# sees the declarations; the texture-decode path is PoC-owned (the SDK never
# decodes bytes — it surfaces URLs verbatim / pixels inline only).
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
add_library(poc_stb_image STATIC "${STB_DIR}/stb_image_impl.cpp")
target_include_directories(poc_stb_image PUBLIC "${STB_DIR}")

add_executable(x3d_poc_renderer
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp")

target_link_libraries(x3d_poc_renderer
    PRIVATE
        x3d_cpp::x3d_cpp
        x3d_stbtt          # T-TEXT glyph atlas (StbttGlyphAtlas) for Text rendering
        x3d_plmpeg         # MovieDecoder backend A (pl_mpeg) for MovieTexture (ADR-0041)
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

# Surface the asset/shaders dir to the binary so it can find the bundled
# hand-written triangle scene + GLSL without a working-directory dance.
target_compile_definitions(x3d_poc_renderer PRIVATE
    # glad is THE GL loader; GLFW must never pull in the system <GL/gl.h> (it isn't
    # present on a runtime-only mesa install, e.g. CI). Defining this on the target
    # — not per-file — protects every TU regardless of include order, so a new file
    # that includes <GLFW/glfw3.h> without glad first can't reintroduce the break.
    GLFW_INCLUDE_NONE
    "X3D_POC_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
    "X3D_POC_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
    # Swap-test seam (U1-U4): the portable UsdPreviewSurface shader lives in the
    # sibling cpu_raster example so ONE source can be validated both by the CPU
    # rasterizer's GlslInterpreter and here on real desktop GL. Opt-in only via
    # --pbr-shader / X3D_POC_PBR_SHADER; default path (pbr.frag) is unaffected.
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
