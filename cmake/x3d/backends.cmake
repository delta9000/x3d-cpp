# cmake/x3d/backends.cmake — flag-gated backends for each seam (physics, script, asset, texture, font, movie) + their swap-tests.

# §25.2.3 ellipsoid projections and optional WGS84 geoid (ADR-0053).
option(X3D_CPP_BUILD_PROJ "Build PROJ GeoProjection backend (OFF default)" OFF)
if(X3D_CPP_BUILD_PROJ)
    find_package(PROJ REQUIRED)
    add_library(x3d_proj STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/proj/ProjGeoProjection.cpp")
    target_link_libraries(x3d_proj PUBLIC x3d_cpp::x3d_cpp PRIVATE PROJ::proj)
    target_include_directories(x3d_proj PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/proj"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math")
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        add_executable(x3d_proj_geo_swap
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/proj/tests/proj_geo_swap_test.cpp")
        target_link_libraries(x3d_proj_geo_swap PRIVATE x3d_proj)
        target_compile_definitions(x3d_proj_geo_swap PRIVATE
            X3D_PROJ_TEST_GRID="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/proj/tests/sample_geoid.gtx")
        add_test(NAME x3d_proj_geo_swap COMMAND x3d_proj_geo_swap)
    endif()
endif()

# Optional Jolt implementation of PhysicsBackend.
option(X3D_CPP_BUILD_PHYSICS "Build Jolt Physics backend (OFF default; fetches Jolt v5.5.0)" OFF)

if(X3D_CPP_BUILD_PHYSICS)
    include(FetchContent)
    # Jolt honors normal variables (CMP0077); keep these defaults in our subtree
    # while preserving the parent's cache. Disable upstream flag overrides,
    # -Werror, and LTO to avoid applying them to SDK compilation.
    set(BUILD_SHARED_LIBS OFF)
    set(OVERRIDE_CXX_FLAGS OFF)
    set(ENABLE_ALL_WARNINGS OFF)
    set(INTERPROCEDURAL_OPTIMIZATION OFF)
    set(ENABLE_INSTALL OFF)
    set(TARGET_UNIT_TESTS OFF)
    set(TARGET_HELLO_WORLD OFF)
    set(TARGET_PERFORMANCE_TEST OFF)
    set(TARGET_SAMPLES OFF)
    set(TARGET_VIEWER OFF)
    set(GENERATE_DEBUG_SYMBOLS OFF)
    # Jolt's CMakeLists lives in Build/ (SOURCE_SUBDIR), and its target is "Jolt".
    FetchContent_Declare(JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build)
    FetchContent_MakeAvailable(JoltPhysics)

    add_library(x3d_physics_jolt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/jolt/JoltBackend.cpp")
    # Keep Jolt architecture flags private: leaking -mfma changes rounding in
    # unrelated code. The static archive still links transitively.
    target_link_libraries(x3d_physics_jolt PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_physics_jolt PRIVATE Jolt)
    target_include_directories(x3d_physics_jolt PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/jolt")
    target_compile_definitions(x3d_physics_jolt PUBLIC X3D_HAVE_PHYSICS=1)
    # Jolt headers are warning-noisy; silence them on this TU only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_physics_jolt PRIVATE -w)
    endif()

    # Physics tests (only when the test build is also on). enable_testing() runs
    # in the X3D_CPP_BUILD_TESTS block below; CTest tolerates add_test before it.
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # (a) JoltBackend free-fall + rest-on-ground + determinism (engine proof).
        add_executable(x3d_physics_jolt_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/jolt_backend_test.cpp")
        target_link_libraries(x3d_physics_jolt_test PRIVATE x3d_physics_jolt)
        add_test(NAME x3d_physics_jolt COMMAND x3d_physics_jolt_test)

        # (b) PhysicsSystem unit: §37 scene -> attach + tick -> RigidBody.position
        #     updated and position_changed delivered to a routed Transform.
        add_executable(x3d_physics_system_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/physics_system_test.cpp")
        target_link_libraries(x3d_physics_system_test PRIVATE x3d_physics_jolt)
        add_test(NAME x3d_physics_system COMMAND x3d_physics_system_test)
    endif()
endif()

# Optional QuickJS ScriptEngine implementation (ADR-0022).
option(X3D_CPP_BUILD_QUICKJS "Build QuickJS ScriptEngine backend (OFF default)" OFF)

if(X3D_CPP_BUILD_QUICKJS)
    include(FetchContent)
    # quickjs-ng predates CMP0077; ask its option() calls to honor our scoped
    # defaults without rewriting the parent's cache. The engine builds as
    # a static lib only (no shared, no examples/libc/install) and cannot clobber
    # our build. Its main library target is `qjs` (see its CMakeLists).
    set(BUILD_SHARED_LIBS OFF)
    set(QJS_BUILD_EXAMPLES OFF)
    set(QJS_BUILD_LIBC OFF)
    set(QJS_ENABLE_INSTALL OFF)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    FetchContent_Declare(quickjs_ng
        GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
        GIT_TAG v0.15.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(quickjs_ng)

    add_library(x3d_quickjs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/QuickJsBackend.cpp")
    # Keep engine headers and flags private to the implementation.
    target_link_libraries(x3d_quickjs PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_quickjs PRIVATE qjs)
    target_include_directories(x3d_quickjs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script")
    # quickjs-ng headers are warning-noisy; silence them on this TU only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_quickjs PRIVATE -w)
    endif()

    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # Lifecycle: load/init/shutdown/prepareEvents/invoke/eventsProcessed,
        # syntax-error -> kInvalidScriptHandle, independent per-script contexts.
        add_executable(x3d_quickjs_backend_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/quickjs_backend_test.cpp")
        target_link_libraries(x3d_quickjs_backend_test PRIVATE x3d_quickjs)
        add_test(NAME x3d_quickjs_backend COMMAND x3d_quickjs_backend_test)
    endif()
endif()

# Optional libcurl AssetResolver; keep backend dependencies private.
option(X3D_CPP_BUILD_CURL "Build libcurl HTTP AssetResolver backend (OFF default)" OFF)

if(X3D_CPP_BUILD_CURL)
    find_package(CURL REQUIRED)
    add_library(x3d_curl STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/HttpResolver.cpp")
    target_link_libraries(x3d_curl PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_curl PRIVATE CURL::libcurl)
    target_include_directories(x3d_curl PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl")
    # libcurl headers are warning-noisy on some toolchains; silence on this TU.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_curl PRIVATE -w)
    endif()

    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        add_executable(x3d_assetresolver_backend_a_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/tests/asset_resolver_backend_a_test.cpp")
        target_link_libraries(x3d_assetresolver_backend_a_test PRIVATE x3d_curl)
        add_test(NAME x3d_assetresolver_backend_a COMMAND x3d_assetresolver_backend_a_test)
        add_executable(x3d_http_address_guard_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/tests/http_address_guard_test.cpp")
        target_link_libraries(x3d_http_address_guard_test PRIVATE x3d_cpp::x3d_cpp CURL::libcurl)
        add_test(NAME x3d_http_address_guard COMMAND x3d_http_address_guard_test)
    endif()
endif()

# Check HTTP fixture bytes and missing assets through the AssetResolver seam.
if(TARGET x3d_curl AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_assetresolver_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/asset_resolver_swap_test.cpp")
    target_link_libraries(x3d_assetresolver_swap PRIVATE x3d_curl)
    target_compile_definitions(x3d_assetresolver_swap PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures")
    add_test(NAME x3d_assetresolver_swap COMMAND x3d_assetresolver_swap)
endif()

# Local-file reads are confined to the permitted root (ADR-0038).
if(X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_fileresolver_test
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/file_resolver_test.cpp")
    target_link_libraries(x3d_fileresolver_test PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_compile_definitions(x3d_fileresolver_test PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures")
    add_test(NAME x3d_fileresolver_test COMMAND x3d_fileresolver_test)
endif()

# Optional stb_image decoder (ADR-0024).
option(X3D_CPP_BUILD_STB "Build stb_image TextureResolver decode backend (OFF default)" OFF)

# assimp ImportSource backend for the asset-import consumer. Gated OFF by
# default (system-only dependency; not vendored). When ON, the
# AssimpSource.cpp backend is compiled into x3d_asset_import and the
# assimp_source_test runs against a committed tiny .obj fixture.
# Build with: cmake -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_ASSIMP=ON
option(X3D_CPP_BUILD_ASSIMP "Build the assimp ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_USD "Build the USD ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_CGLTF "Build the cgltf glTF ImportSource backend for asset_import (header-only, MIT; ON default)" ON)

if(X3D_CPP_BUILD_STB)
    add_library(x3d_stb STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb/StbTextureResolver.cpp")
    target_link_libraries(x3d_stb PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stb PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb")
    target_include_directories(x3d_stb PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/dds")
    # stb_image.h is PRIVATE: only StbTextureResolver.cpp needs it; the factory
    # returns a std::function exchanging std types, so consumers linking x3d_stb
    # inherit no stb headers (decoder-free StbTextureResolver.hpp).
    target_include_directories(x3d_stb PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/poc_renderer/third_party")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stb PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# Optional Wuffs decoder (ADR-0024). Compile its amalgamation in one translation
# unit to avoid duplicate codec definitions.
option(X3D_CPP_BUILD_WUFFS "Build wuffs TextureResolver decode backend (OFF default)" OFF)

if(X3D_CPP_BUILD_WUFFS)
    add_library(x3d_wuffs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs/WuffsTextureResolver.cpp")
    target_link_libraries(x3d_wuffs PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_wuffs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs")
    target_include_directories(x3d_wuffs PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/dds")
    # The wuffs amalgamation lives under runtime/io/wuffs/vendor and is included
    # by the TU relative to its own dir (PRIVATE — decoder-free public header).
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_wuffs PRIVATE -w)  # vendored amalgamation.
    endif()
endif()

# Optional stb_truetype glyph metrics (ADR-0025).
option(X3D_CPP_BUILD_STBTT "Build stb_truetype FontMetrics backend (OFF default)" OFF)

# The PoC renderer consumes the stbtt glyph atlas (StbttGlyphAtlas) to render
# Text, so build this backend whenever the PoC is on, not only on explicit opt-in.
if(X3D_CPP_BUILD_STBTT OR X3D_CPP_BUILD_POC)
    add_library(x3d_stbtt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/StbttFontMetrics.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/StbttGlyphAtlas.cpp")
    target_link_libraries(x3d_stbtt PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stbtt PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt")
    # stb_truetype.h is PRIVATE: only StbttFontMetrics.cpp needs it; the factory
    # returns a std::function exchanging std types (decoder-free header).
    target_include_directories(x3d_stbtt PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stbtt PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# Test the raster helper against local font fixtures.
if(TARGET x3d_stbtt AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_text_atlas_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/glyph_atlas_tests.cpp")
    target_link_libraries(x3d_text_atlas_tests PRIVATE x3d_stbtt x3d_doctest_main)
    target_include_directories(x3d_text_atlas_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/vendor")
    target_compile_definitions(x3d_text_atlas_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts")
    add_test(NAME x3d_text_atlas_tests COMMAND x3d_text_atlas_tests)
    set_tests_properties(x3d_text_atlas_tests PROPERTIES TIMEOUT 120)
endif()

# Optional pl_mpeg MPEG-1 decoder (ADR-0041).
option(X3D_CPP_BUILD_PLMPEG "Build pl_mpeg MovieDecoder backend (OFF default)" OFF)

# The PoC renderer consumes the pl_mpeg decoder to play MovieTextures, so build
# this backend whenever the PoC is on, not only on explicit opt-in.
if(X3D_CPP_BUILD_PLMPEG OR X3D_CPP_BUILD_POC)
    add_library(x3d_plmpeg STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/PlMpegMovieDecoder.cpp")
    target_link_libraries(x3d_plmpeg PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_plmpeg PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg")
    # pl_mpeg.h is PRIVATE: only PlMpegMovieDecoder.cpp needs it; the factory
    # returns a std::function exchanging std types (decoder-free header).
    target_include_directories(x3d_plmpeg PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_plmpeg PRIVATE -w)  # pl_mpeg is warning-noisy.
    endif()
endif()

# Check glyph metrics on local font fixtures (ADR-0025).
if(TARGET x3d_stbtt AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_text_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/font_metrics_tests.cpp")
    target_link_libraries(x3d_text_tests PRIVATE
        x3d_stbtt x3d_doctest_main)
    target_include_directories(x3d_text_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_text_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts")
    add_test(NAME x3d_text_tests COMMAND x3d_text_tests)
    set_tests_properties(x3d_text_tests PROPERTIES TIMEOUT 120)
endif()

# Compare decoded pixels on local image fixtures (ADR-0024).
if(TARGET x3d_stb AND TARGET x3d_wuffs AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_texture_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/texture_decode_tests.cpp")
    target_link_libraries(x3d_texture_tests PRIVATE
        x3d_stb x3d_wuffs x3d_doctest_main)
    target_include_directories(x3d_texture_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_texture_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures/texture"
        TEMP_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME x3d_texture_tests COMMAND x3d_texture_tests)
    set_tests_properties(x3d_texture_tests PROPERTIES TIMEOUT 120)
endif()

# Apply the decoder lifecycle contract to each reference clip (ADR-0041).
if(TARGET x3d_plmpeg AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_movie_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/movie_decoder_tests.cpp")
    target_link_libraries(x3d_movie_tests PRIVATE x3d_plmpeg x3d_doctest_main)
    target_include_directories(x3d_movie_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_movie_tests PRIVATE
        PLMPEG_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/fixtures")
    add_test(NAME x3d_movie_tests COMMAND x3d_movie_tests)
    set_tests_properties(x3d_movie_tests PROPERTIES TIMEOUT 120)
endif()
