# cmake/x3d/backends.cmake — flag-gated backends for each seam (physics, script, asset, texture, font, movie) + their swap-tests.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Jolt Physics backend (runtime/physics/jolt/). The flag-gated example backend
# for the engine-agnostic PhysicsBackend seam (runtime/physics/PhysicsBackend.hpp
# + PhysicsSystem.hpp are CORE, header-only, Jolt-free, always present). Default
# OFF so the standard build/golden/ctest path is COMPLETELY unaffected: with this
# OFF Jolt is never fetched and the backend lib never built/linked. Mirrors the
# x3d_duktape isolation rule. Build with:
#   cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_PHYSICS "Build Jolt Physics backend (OFF default; fetches Jolt v5.5.0)" OFF)

if(X3D_CPP_BUILD_PHYSICS)
    include(FetchContent)
    # Jolt's CMake honors these toggles; force them so they cannot clobber our
    # flags (OVERRIDE_CXX_FLAGS), apply -Werror (ENABLE_ALL_WARNINGS), or trip an
    # LTO archive-format mismatch at link (INTERPROCEDURAL_OPTIMIZATION).
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
    set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
    set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
    set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
    set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
    set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
    set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
    set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
    # Jolt's CMakeLists lives in Build/ (SOURCE_SUBDIR), and its target is "Jolt".
    FetchContent_Declare(JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build)
    FetchContent_MakeAvailable(JoltPhysics)

    # The JoltBackend: the single TU where Jolt meets the seam. Static lib so the
    # rest of the build is untouched — only the physics tests / x3d_cli link it.
    add_library(x3d_physics_jolt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/jolt/JoltBackend.cpp")
    # Jolt is PRIVATE: its headers + (critically) its PUBLIC arch flags
    # (-mavx2/-mfma/...) are needed only to compile JoltBackend.cpp, NOT by our
    # consumers. JoltBackend.hpp is Jolt-free (pImpl), so x3d_cli/tests linking
    # x3d_physics_jolt must NOT inherit -mfma — FMA contraction would silently
    # change FP rounding in unrelated (non-physics) code (it shifted a
    # ProximitySensor knife-edge by one tick). The Jolt static archive is still
    # linked transitively because x3d_physics_jolt depends on it.
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
        #     updated AND position_changed delivered to a routed Transform.
        add_executable(x3d_physics_system_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/physics_system_test.cpp")
        target_link_libraries(x3d_physics_system_test PRIVATE x3d_physics_jolt)
        add_test(NAME x3d_physics_system COMMAND x3d_physics_system_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# QuickJS ScriptEngine backend (runtime/script/QuickJsBackend.{hpp,cpp}). The
# flag-gated SECOND backend for the language-agnostic ScriptEngine seam
# (runtime/script/ScriptEngine.hpp is CORE, header-only, JS-free, always
# present). This is the Phase-0 genericity pilot: a fully independent backend
# (quickjs-ng) implementing the SAME interface as the Duktape EcmaScriptBackend,
# with zero core changes, to prove the seam is generic.
#
# Mirrors the x3d_physics_jolt isolation rule exactly: option() -> FetchContent
# -> an isolated STATIC lib (x3d_quickjs) whose single TU is QuickJsBackend.cpp;
# quickjs-ng linked PRIVATE so its headers/flags never leak to consumers
# (QuickJsBackend.hpp is QuickJS-free via a pImpl, like JoltBackend.hpp is
# Jolt-free). Default OFF so the standard build/golden/ctest path is COMPLETELY
# unaffected: with this OFF quickjs-ng is never fetched and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_QUICKJS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_QUICKJS "Build QuickJS ScriptEngine backend (OFF default)" OFF)

if(X3D_CPP_BUILD_QUICKJS)
    include(FetchContent)
    # quickjs-ng's CMake honors these toggles; force them so the engine builds as
    # a static lib only (no shared, no examples/libc/install) and cannot clobber
    # our build. Its main library target is `qjs` (see its CMakeLists).
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(QJS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QJS_BUILD_LIBC OFF CACHE BOOL "" FORCE)
    set(QJS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(quickjs_ng
        GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
        GIT_TAG v0.15.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(quickjs_ng)

    # The QuickJsBackend: the single TU where quickjs-ng meets the seam. Static
    # lib so the rest of the build is untouched — only the quickjs test / x3d_cli
    # link it.
    add_library(x3d_quickjs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/QuickJsBackend.cpp")
    # qjs is PRIVATE: its headers (quickjs.h) are needed only to compile
    # QuickJsBackend.cpp, NOT by our consumers. QuickJsBackend.hpp is QuickJS-free
    # (pImpl), so x3d_cli/tests linking x3d_quickjs must NOT inherit QuickJS's
    # include dirs or any flags. The qjs static archive is still linked
    # transitively because x3d_quickjs depends on it.
    target_link_libraries(x3d_quickjs PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_quickjs PRIVATE qjs)
    target_include_directories(x3d_quickjs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script")
    # quickjs-ng headers are warning-noisy; silence them on this TU only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_quickjs PRIVATE -w)
    endif()

    # QuickJS backend test (only when the test build is also on). enable_testing()
    # runs in the X3D_CPP_BUILD_TESTS block below; CTest tolerates add_test before
    # it (same precedent as the physics tests above).
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # U1 lifecycle: load/init/shutdown/prepareEvents/invoke/eventsProcessed,
        # syntax-error -> kInvalidScriptHandle, independent per-script contexts.
        add_executable(x3d_quickjs_backend_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/quickjs_backend_test.cpp")
        target_link_libraries(x3d_quickjs_backend_test PRIVATE x3d_quickjs)
        add_test(NAME x3d_quickjs_backend COMMAND x3d_quickjs_backend_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# Mirrors the x3d_quickjs isolation rule: option() -> find_package -> an
# isolated STATIC lib (x3d_curl) whose single TU is HttpResolver.cpp; libcurl
# linked PRIVATE so its headers/flags never leak to consumers. Default OFF so
# the standard build/golden/ctest path is COMPLETELY unaffected: with this OFF
# libcurl is never linked and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_CURL=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_CURL "Build libcurl HTTP AssetResolver backend (OFF default)" OFF)

if(X3D_CPP_BUILD_CURL)
    find_package(CURL REQUIRED)
    # The single TU where libcurl meets the seam. Static lib so the rest of
    # the build is untouched — only the curl backend test / future swap-test
    # link it.
    add_library(x3d_curl STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/HttpResolver.cpp")
    # CURL::libcurl is PRIVATE: its headers (curl.h) are needed only to
    # compile HttpResolver.cpp, NOT by our consumers. HttpResolver.hpp is
    # libcurl-free (pure std::function AssetResolver return), so tests
    # linking x3d_curl do NOT inherit libcurl's include dirs or any flags.
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
        # U1 per-backend test: URL prefix check + libcurl error path. The
        # success path is tested in U3 (swap-test) against an in-process
        # HTTP server, keeping this test fast + hermetic + offline.
        add_executable(x3d_assetresolver_backend_a_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/tests/asset_resolver_backend_a_test.cpp")
        target_link_libraries(x3d_assetresolver_backend_a_test PRIVATE x3d_curl)
        add_test(NAME x3d_assetresolver_backend_a COMMAND x3d_assetresolver_backend_a_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# Mirrors the x3d_quickjs / x3d_curl isolation rule: option() -> find_package
# -> an isolated STATIC lib (x3d_s3) whose single TU is S3Resolver.cpp; AWS
# SDK linked PRIVATE so its headers/flags never leak to consumers. Default OFF
# so the standard build/golden/ctest path is COMPLETELY unaffected: with this
# OFF the AWS SDK is never linked and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_S3=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_S3 "Build AWS S3 SDK AssetResolver backend (OFF default)" OFF)

if(X3D_CPP_BUILD_S3)
    find_package(AWSSDK REQUIRED COMPONENTS s3)
    # The single TU where the AWS SDK meets the seam. Static lib so the rest
    # of the build is untouched — only the S3 backend test / future swap-test
    # link it.
    add_library(x3d_s3 STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/S3Resolver.cpp")
    # AWS SDK targets are PRIVATE: their headers (aws/s3/S3Client.h etc.) are
    # needed only to compile S3Resolver.cpp, NOT by our consumers.
    # S3Resolver.hpp is AWS-SDK-free (pure std::function AssetResolver return),
    # so tests linking x3d_s3 do NOT inherit the AWS SDK's include dirs or any
    # flags.
    target_link_libraries(x3d_s3 PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_s3 PRIVATE
        aws-cpp-sdk-core
        aws-cpp-sdk-s3)
    target_include_directories(x3d_s3 PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3")
    # AWS SDK headers are warning-noisy on some toolchains; silence on this TU.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_s3 PRIVATE -w)
    endif()

    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # U2 per-backend test: URL prefix check + URL parse + SDK init path.
        # The success path is tested in U3 (swap-test) against a docker minio
        # fixture, keeping this test fast + hermetic + offline.
        add_executable(x3d_assetresolver_backend_b_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/tests/asset_resolver_backend_b_test.cpp")
        target_link_libraries(x3d_assetresolver_backend_b_test PRIVATE x3d_s3)
        add_test(NAME x3d_assetresolver_backend_b COMMAND x3d_assetresolver_backend_b_test)

        # Test-support lib: the swap-test (U3) must seed a fixture bucket so
        # Backend B has something to fetch. The seeding uses the AWS SDK, so it
        # is confined to its own TU here (AWS SDK linked PRIVATE, like x3d_s3)
        # rather than called from the swap-test TU directly — otherwise the AWS
        # SDK would appear in two TUs that can disagree on headers and corrupt
        # ClientConfiguration at runtime. See runtime/io/s3/S3TestSupport.hpp.
        add_library(x3d_s3_testsupport STATIC
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/S3TestSupport.cpp")
        target_link_libraries(x3d_s3_testsupport PUBLIC x3d_cpp::x3d_cpp)
        target_link_libraries(x3d_s3_testsupport PRIVATE
            aws-cpp-sdk-core aws-cpp-sdk-s3)
        target_include_directories(x3d_s3_testsupport PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(x3d_s3_testsupport PRIVATE -w)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# U3 — the AssetResolver/IO genericity proof. The swap-test ctest drives
# identical fixture bytes through Backend A (libcurl HTTP) and Backend B
# (AWS S3) and asserts byte-equal AssetResult.bytes, plus equal Failed
# status for missing keys. Gated by both backends being built.
# Mirrors the x3d_quickjs_swap precedent (cmake/x3d/cli.cmake).
# Build: cmake --preset dev -DX3D_CPP_BUILD_CURL=ON -DX3D_CPP_BUILD_S3=ON
# Run:   ctest -R x3d_assetresolver_swap  (needs $X3D_S3_ENDPOINT pointing
#         at a docker minio with bucket 'x3d-swap-fixtures'; otherwise the
#         test degrades to an HTTP-only parity check with a SKIP message).
# ---------------------------------------------------------------------------
if(TARGET x3d_curl AND TARGET x3d_s3 AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_assetresolver_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/asset_resolver_swap_test.cpp")
    # The swap-test talks only through the seams (makeHttpResolver/makeS3Resolver)
    # plus the AWS-free x3d_s3_testsupport seeding helper. It never includes
    # <aws/...> or <curl/...> itself, so no heavy backend dependency leaks into
    # the test TU (both backends' libs link their dep PRIVATE).
    target_link_libraries(x3d_assetresolver_swap PRIVATE
        x3d_curl x3d_s3 x3d_s3_testsupport)
    target_compile_definitions(x3d_assetresolver_swap PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures")
    add_test(NAME x3d_assetresolver_swap COMMAND x3d_assetresolver_swap)
endif()

# ---------------------------------------------------------------------------
# FileResolver — SEC-3-confined local-file AssetResolver backend (Task 1, #80).
# Header-only backend that answers Ready/Failed for file: protocol URLs.
# Run: ctest --preset dev -R x3d_fileresolver
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# TextureResolver decode seam — Backend A (stb_image). ADR-0024 / U1.
# Mirrors the x3d_curl isolation rule: option() -> an isolated STATIC lib
# (x3d_stb) whose single TU is StbTextureResolver.cpp; stb_image.h is included
# PRIVATE so it meets the seam in ONE TU and never leaks. With this OFF the lib
# is never built and stb is never compiled into the runtime.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_STB=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_STB "Build stb_image TextureResolver decode backend (OFF default)" OFF)

# ---------------------------------------------------------------------------
# assimp ImportSource backend for the asset-import consumer. Gated OFF by
# default (system-only dependency; not vendored). When ON, the
# AssimpSource.cpp backend is compiled into x3d_asset_import and the
# assimp_source_test runs against a committed tiny .obj fixture.
# Build with: cmake -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_ASSIMP=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_ASSIMP "Build the assimp ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_USD "Build the USD ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_CGLTF "Build the cgltf glTF ImportSource backend for asset_import (header-only, MIT; ON default)" ON)

if(X3D_CPP_BUILD_STB)
    add_library(x3d_stb STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb/StbTextureResolver.cpp")
    target_link_libraries(x3d_stb PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stb PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb")
    # stb_image.h is PRIVATE: only StbTextureResolver.cpp needs it; the factory
    # returns a std::function exchanging std types, so consumers linking x3d_stb
    # inherit no stb headers (decoder-free StbTextureResolver.hpp).
    target_include_directories(x3d_stb PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/poc_renderer/third_party")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stb PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# ---------------------------------------------------------------------------
# TextureResolver decode seam — Backend B (wuffs). ADR-0024 / U2.
# wuffs is a vendored single-file amalgamation; WUFFS_IMPLEMENTATION is defined
# in the one TU (WuffsTextureResolver.cpp) which #includes it PRIVATE, so the
# 1.8 MB codec body compiles into exactly one object file and never leaks.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_WUFFS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_WUFFS "Build wuffs TextureResolver decode backend (OFF default)" OFF)

if(X3D_CPP_BUILD_WUFFS)
    add_library(x3d_wuffs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs/WuffsTextureResolver.cpp")
    target_link_libraries(x3d_wuffs PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_wuffs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs")
    # The wuffs amalgamation lives under runtime/io/wuffs/vendor and is included
    # by the TU relative to its own dir (PRIVATE — decoder-free public header).
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_wuffs PRIVATE -w)  # vendored amalgamation.
    endif()
endif()

# ---------------------------------------------------------------------------
# FontMetrics seam — Backend A (stb_truetype). ADR-0025 / U1.
# option() -> an isolated STATIC lib (x3d_stbtt) whose single TU is
# StbttFontMetrics.cpp; stb_truetype.h is included PRIVATE so it meets the seam
# in ONE TU and never leaks. Build: cmake --preset dev -DX3D_CPP_BUILD_STBTT=ON
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# MovieDecoder seam — Backend A (pl_mpeg, MPEG-1). ADR-0041.
# option() -> an isolated STATIC lib (x3d_plmpeg) whose single TU is
# PlMpegMovieDecoder.cpp; pl_mpeg.h is included PRIVATE so the codec meets the
# seam in ONE TU and never leaks. MPEG-1 is royalty-free + pl_mpeg is MIT, so it
# ships default-OFF (encumbered codecs plug into the same seam downstream).
# Build: cmake --preset dev -DX3D_CPP_BUILD_PLMPEG=ON
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# MovieDecoder seam — Backend B (libtheora/libogg). ADR-0041.
# The SECOND independent backend — it makes the seam GENERIC (one frozen interface,
# two format-partitioned codecs) via the shared semantics-contract test. Theora is
# a Xiph BSD codec and royalty-free, so it joins the default-shippable tier
# (flag-gated OFF). libtheora/libogg are NOT single-header; they are found via
# pkg-config and linked PRIVATE to the one TU (the FreeType backend-B pattern), so
# no codec header leaks to consumers.
# Build: cmake --preset dev -DX3D_CPP_BUILD_THEORA=ON  (needs libtheora-dev + libogg-dev)
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_THEORA "Build libtheora MovieDecoder backend (OFF default)" OFF)

if(X3D_CPP_BUILD_THEORA)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(THEORADEC REQUIRED IMPORTED_TARGET theoradec)
    add_library(x3d_theora STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora/TheoraMovieDecoder.cpp")
    target_link_libraries(x3d_theora PUBLIC x3d_cpp::x3d_cpp)
    # theoradec.pc pulls in libogg transitively; both stay PRIVATE to this TU.
    target_link_libraries(x3d_theora PRIVATE PkgConfig::THEORADEC)
    target_include_directories(x3d_theora PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora")
endif()

# ---------------------------------------------------------------------------
# FontMetrics seam — Backend B (FreeType). ADR-0025 / U2.
# FreeType is NOT single-header; it is provided via vcpkg (find_package). Its
# headers are PRIVATE to the one TU (FreetypeFontMetrics.cpp), so no FreeType
# header leaks to consumers. Build: -DX3D_CPP_BUILD_FREETYPE=ON with the vcpkg
# toolchain file (see the CI job / spec §6).
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_FREETYPE "Build FreeType FontMetrics backend (OFF default)" OFF)

if(X3D_CPP_BUILD_FREETYPE)
    find_package(Freetype REQUIRED)
    add_library(x3d_freetype STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype/FreetypeFontMetrics.cpp")
    target_link_libraries(x3d_freetype PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_freetype PRIVATE Freetype::Freetype)  # heavy dep PRIVATE
    target_include_directories(x3d_freetype PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype")
endif()

# ---------------------------------------------------------------------------
# U3 — the FontMetrics genericity proof. One grouped doctest binary drives
# identical font fixtures through stb_truetype and FreeType and asserts
# EXACT-equal advanceEm + Failed-parity. Gated on both backends.
# Run: ctest -R x3d_text   (hermetic + offline — local font files only).
# ---------------------------------------------------------------------------
if(TARGET x3d_stbtt AND TARGET x3d_freetype AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_text_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/font_metrics_tests.cpp")
    target_link_libraries(x3d_text_tests PRIVATE
        x3d_stbtt x3d_freetype x3d_doctest_main)
    target_include_directories(x3d_text_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_text_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts")
    add_test(NAME x3d_text_tests COMMAND x3d_text_tests)
    set_tests_properties(x3d_text_tests PROPERTIES TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# U3 — the TextureResolver decode genericity proof. One grouped doctest binary
# drives identical image fixtures (PNG/BMP/GIF/TGA) through Backend A (stb) and
# Backend B (wuffs) and asserts BYTE-EQUAL RGBA + Failed-parity, plus the U1/U2
# per-backend cases and the U2.5 sniff-dispatch composer. Gated on both backends.
# Build: cmake --preset dev -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON
# Run:   ctest -R x3d_texture   (hermetic + offline — no services, no fixtures
#        to seed; the decoders take local bytes directly).
# ---------------------------------------------------------------------------
if(TARGET x3d_stb AND TARGET x3d_wuffs AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_texture_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/texture_decode_tests.cpp")
    # The test TU talks only through the seam factories (makeStb/Wuffs/...); it
    # links the backend libs but includes NO decoder header, so no heavy backend
    # header can leak in and disagree with the linked lib (the AssetResolver
    # ABI-skew lesson — trivially clean here since the factories exchange std).
    target_link_libraries(x3d_texture_tests PRIVATE
        x3d_stb x3d_wuffs x3d_doctest_main)
    target_include_directories(x3d_texture_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_texture_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures/texture")
    add_test(NAME x3d_texture_tests COMMAND x3d_texture_tests)
    set_tests_properties(x3d_texture_tests PROPERTIES TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# MovieDecoder seam — genericity proof (ADR-0041). One doctest binary runs the
# SAME semantics-contract (runContract) against each backend's OWN reference clip:
# pl_mpeg / MPEG-1 (Backend A, a 686-byte raw elementary stream — the NIST corpus
# shape) and, when built, libtheora / Ogg-Theora (Backend B). Two backends passing
# the identical contract (Ready dims + tight opaque RGBA, EOF-holds-last, backward-
# seek rewind, Pending/Failed/garbage parity) is what makes the seam GENERIC.
# Hermetic + offline (committed fixtures). Run: ctest -R x3d_movie
# ---------------------------------------------------------------------------
if(TARGET x3d_plmpeg AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_movie_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/movie_decoder_tests.cpp")
    # Talks ONLY through the seam factories (decoder-free headers); no codec header.
    target_link_libraries(x3d_movie_tests PRIVATE x3d_plmpeg x3d_doctest_main)
    target_include_directories(x3d_movie_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_movie_tests PRIVATE
        PLMPEG_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/fixtures")
    # Backend B joins the SAME binary when built -> the genericity proof runs both.
    if(TARGET x3d_theora)
        target_link_libraries(x3d_movie_tests PRIVATE x3d_theora)
        target_include_directories(x3d_movie_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora")
        target_compile_definitions(x3d_movie_tests PRIVATE
            X3D_MOVIE_HAVE_THEORA=1
            THEORA_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora/tests/fixtures")
    endif()
    add_test(NAME x3d_movie_tests COMMAND x3d_movie_tests)
    set_tests_properties(x3d_movie_tests PROPERTIES TIMEOUT 120)
endif()
