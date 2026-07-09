# ---------------------------------------------------------------------------
# Asset-import authoring footprint smoke. Builds a tiny consumer against ONLY
# x3d_cpp::authoring to prove the slim surface links and serializes a scene.
# Task 11 finalizes the full asset-import subdirectory.
# ---------------------------------------------------------------------------
add_executable(x3d_authoring_smoke
    "${CMAKE_CURRENT_SOURCE_DIR}/footprint/authoring_smoke.cpp")
target_link_libraries(x3d_authoring_smoke PRIVATE x3d_cpp::authoring)
target_compile_options(x3d_authoring_smoke PRIVATE -ffunction-sections -fdata-sections)
target_link_options(x3d_authoring_smoke PRIVATE -Wl,--gc-sections -Wl,-Map=$<TARGET_FILE:x3d_authoring_smoke>.map)
add_test(NAME x3d_assetimport_authoring_smoke COMMAND x3d_authoring_smoke)


# ---------------------------------------------------------------------------
# ImportScene IR + ImportSource seam, backed by the dependency-free
# FixtureSource (Task 4) — synthetic scenes for CI without assimp and for
# every emit test (Tasks 5-8).
# ---------------------------------------------------------------------------
add_executable(x3d_assetimport_fixture_source
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_source.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixture_source_test.cpp")
target_include_directories(x3d_assetimport_fixture_source PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support")
target_compile_features(x3d_assetimport_fixture_source PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_fixture_source COMMAND x3d_assetimport_fixture_source)

# ---------------------------------------------------------------------------
# BackendRegistry: pure priority/name selection over ImportSource backends.
# Unit-tested with fake backends — no concrete backend build gate required.
# ---------------------------------------------------------------------------
add_executable(x3d_assetimport_backend_registry
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/backend_registry_test.cpp")
target_include_directories(x3d_assetimport_backend_registry PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support")
target_compile_features(x3d_assetimport_backend_registry PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_backend_registry COMMAND x3d_assetimport_backend_registry)

# ---------------------------------------------------------------------------
# emit(): ImportScene IR -> x3d::runtime::X3DDocument, geometry core (Task 5).
# Links ONLY x3d_cpp::authoring — proves the consumer never needs parse/
# execution/extract/sdk to build a document from scratch.
# ---------------------------------------------------------------------------
add_executable(x3d_assetimport_emit
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_source.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/emit_test.cpp")
target_include_directories(x3d_assetimport_emit PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support")
target_link_libraries(x3d_assetimport_emit PRIVATE x3d_cpp::authoring)
target_compile_features(x3d_assetimport_emit PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_emit COMMAND x3d_assetimport_emit)

# ---------------------------------------------------------------------------
# Texture pipeline (Task 9): decode/dedup/re-encode ImportScene textures into
# <outDir>/assets/, returning a TexturePlan emit() threads into ImageTexture
# slots. Decodes through the x3d_stb seam; re-encodes PNG via the vendored
# stb_image_write.h. Only available when -DX3D_CPP_BUILD_STB=ON.
# ---------------------------------------------------------------------------
if(TARGET x3d_stb)
    add_executable(x3d_assetimport_texture
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/texture_pipeline.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/emit.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/fixture_source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/texture_pipeline_test.cpp")
    target_include_directories(x3d_assetimport_texture PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/extract"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/io/stb")
    target_link_libraries(x3d_assetimport_texture PRIVATE x3d_stb x3d_cpp::authoring)
    target_compile_features(x3d_assetimport_texture PRIVATE cxx_std_20)
    add_test(NAME x3d_assetimport_texture COMMAND x3d_assetimport_texture)
else()
    message(STATUS "asset_import: x3d_stb not built (-DX3D_CPP_BUILD_STB=ON); "
                   "skipping the texture_pipeline test target.")
endif()

# ---------------------------------------------------------------------------
# assimp ImportSource backend (Task 10): gated behind X3D_CPP_BUILD_ASSIMP.
# Only compiles/links assimp when the option is ON and find_package(assimp)
# succeeds; otherwise skipped with a clear message (never a hard failure on
# the absence of assimp — Task 13's CI handles the no-assimp job).
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_ASSIMP)
    find_package(assimp CONFIG QUIET)
    if(TARGET assimp::assimp)
        add_executable(x3d_assetimport_assimp
            "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/assimp_source.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/assimp_source_test.cpp")
        target_include_directories(x3d_assetimport_assimp PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support")
        target_link_libraries(x3d_assetimport_assimp PRIVATE assimp::assimp)
        target_compile_features(x3d_assetimport_assimp PRIVATE cxx_std_20)
        target_compile_definitions(x3d_assetimport_assimp PRIVATE
            X3D_ASSET_IMPORT_HAVE_ASSIMP=1
            FIXTURE_OBJ="${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures/tri.obj")
        add_test(NAME x3d_assetimport_assimp COMMAND x3d_assetimport_assimp)
    else()
        message(STATUS "asset_import: X3D_CPP_BUILD_ASSIMP=ON but assimp not "
                       "found via find_package(assimp CONFIG); skipping the "
                       "assimp_source test target. Install assimp or set "
                       "assimp_DIR to its cmake config dir.")
    endif()
else()
    message(STATUS "asset_import: assimp backend disabled "
                   "(-DX3D_CPP_BUILD_ASSIMP=ON to enable).")
endif()

# ---------------------------------------------------------------------------
# USD ImportSource backend: gated behind X3D_CPP_BUILD_USD.
# Compiles/links tinyusdz static library when the option is ON.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_USD "Build USD (USDA/USDC/USDZ) asset import backend" OFF)

if(X3D_CPP_BUILD_USD)
    include(FetchContent)
    FetchContent_Declare(
        tinyusdz
        GIT_REPOSITORY https://github.com/lighttransport/tinyusdz.git
        GIT_TAG 11a2d36
        GIT_SHALLOW TRUE
    )
    
    FetchContent_GetProperties(tinyusdz)
    if(NOT tinyusdz_POPULATED)
        FetchContent_Populate(tinyusdz)

        # Apply our vendored patch to the pinned tinyusdz checkout. It carries:
        #   * build fixes (cstdint include, sizeof(half*) static_asserts,
        #     value::TYPE_ID_1D_ARRAY_BIT qualification) — needed to compile with
        #     our toolchain; formerly inline string(REPLACE) edits.
        #   * USD composition support: `add` references/payloads (LIVRPS), and
        #     recursive subLayer composition of referenced/payloaded assets, so
        #     composed scenes (e.g. OpenChessSet) resolve their geometry instead
        #     of importing as empty Xforms. Plus tolerant MaterialX handling
        #     (skip-with-warning) so unsupported .mtlx arcs don't drop geometry.
        # See patches/tinyusdz-composition.patch (git-format, -p1).
        set(TINYUSDZ_PATCH "${CMAKE_CURRENT_SOURCE_DIR}/patches/tinyusdz-composition.patch")
        # Idempotent: if the reverse patch applies cleanly it's already applied.
        execute_process(
            COMMAND git apply --reverse --check "${TINYUSDZ_PATCH}"
            WORKING_DIRECTORY "${tinyusdz_SOURCE_DIR}"
            RESULT_VARIABLE TINYUSDZ_PATCH_ALREADY
            ERROR_QUIET OUTPUT_QUIET)
        if(NOT TINYUSDZ_PATCH_ALREADY EQUAL 0)
            execute_process(
                COMMAND git apply "${TINYUSDZ_PATCH}"
                WORKING_DIRECTORY "${tinyusdz_SOURCE_DIR}"
                RESULT_VARIABLE TINYUSDZ_PATCH_RESULT)
            if(NOT TINYUSDZ_PATCH_RESULT EQUAL 0)
                message(FATAL_ERROR
                    "Failed to apply ${TINYUSDZ_PATCH} to tinyusdz source at "
                    "${tinyusdz_SOURCE_DIR}")
            endif()
        endif()
    endif()
    
    set(TINYUSDZ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(TINYUSDZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TINYUSDZ_NO_WERROR ON CACHE BOOL "" FORCE)
    
    add_subdirectory(${tinyusdz_SOURCE_DIR} ${tinyusdz_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# ---------------------------------------------------------------------------
# --emit-glsl support: embed the canonical portable UsdPreviewSurface shader
# (examples/cpu_raster/shaders/usd_preview_surface.frag) into a generated
# header via configure_file, so glsl_emit.cpp can text-transform select
# `uniform` lines into `const` without a runtime file dependency on the
# sibling example. Cross-example coupling: cpu_raster owns the frozen shader
# contract; asset_import only reads it at CMake configure time.
# ---------------------------------------------------------------------------
set(X3D_USD_PREVIEW_SURFACE_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/../cpu_raster/shaders/usd_preview_surface.frag")
file(READ "${X3D_USD_PREVIEW_SURFACE_FRAG}" X3D_USD_PREVIEW_SURFACE_FRAG_CONTENT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/usd_preview_surface_embed.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/usd_preview_surface_embed.hpp"
    @ONLY)

# ---------------------------------------------------------------------------
# final target: x3d_asset_import (Task 11)
# ---------------------------------------------------------------------------
set(ASSET_IMPORT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_source.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/texture_pipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl_emit.cpp"
)
set(ASSET_IMPORT_LIBS
    x3d_cpp::authoring
    x3d_cpp::sdk
)
if(TARGET x3d_stb)
    list(APPEND ASSET_IMPORT_LIBS x3d_stb)
endif()

if(X3D_CPP_BUILD_ASSIMP)
    find_package(assimp CONFIG QUIET)
    if(TARGET assimp::assimp)
        list(APPEND ASSET_IMPORT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assimp_source.cpp")
        list(APPEND ASSET_IMPORT_LIBS assimp::assimp)
        set(HAVE_ASSIMP 1)
    else()
        set(HAVE_ASSIMP 0)
    endif()
else()
    set(HAVE_ASSIMP 0)
endif()

if(X3D_CPP_BUILD_USD)
    if(TARGET tinyusdz_static)
        list(APPEND ASSET_IMPORT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/usd_source.cpp")
        list(APPEND ASSET_IMPORT_LIBS tinyusdz_static)
        set(HAVE_USD 1)
    else()
        set(HAVE_USD 0)
    endif()
else()
    set(HAVE_USD 0)
endif()

add_executable(x3d_asset_import ${ASSET_IMPORT_SOURCES})
target_link_libraries(x3d_asset_import PRIVATE ${ASSET_IMPORT_LIBS})
target_compile_features(x3d_asset_import PRIVATE cxx_std_20)
target_include_directories(x3d_asset_import PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/io/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/extract"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
if(HAVE_ASSIMP)
    target_compile_definitions(x3d_asset_import PRIVATE X3D_ASSET_IMPORT_HAVE_ASSIMP=1)
endif()
if(HAVE_USD)
    target_compile_definitions(x3d_asset_import PRIVATE X3D_ASSET_IMPORT_HAVE_USD=1)
    target_include_directories(x3d_asset_import PRIVATE "${tinyusdz_SOURCE_DIR}/src")
endif()

# ---------------------------------------------------------------------------
# USD unit test
# ---------------------------------------------------------------------------
if(HAVE_USD)
    add_executable(x3d_assetimport_usd
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/usd_source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/usd_source_test.cpp")
    target_include_directories(x3d_assetimport_usd PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support"
        "${tinyusdz_SOURCE_DIR}/src")
    target_link_libraries(x3d_assetimport_usd PRIVATE tinyusdz_static)
    target_compile_features(x3d_assetimport_usd PRIVATE cxx_std_20)
    target_compile_definitions(x3d_assetimport_usd PRIVATE
        TEXTURE_USDZ="${tinyusdz_SOURCE_DIR}/models/texture-cat-plane.usdz"
        COMPOSED_ROOT_USDA="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/composed/root.usda"
        MATERIAL_USDA="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/material_params.usda")
    add_test(NAME x3d_assetimport_usd COMMAND x3d_assetimport_usd)
endif()

# ---------------------------------------------------------------------------
# round-trip test: x3d_assetimport_roundtrip (Task 11)
# ---------------------------------------------------------------------------
add_executable(x3d_assetimport_roundtrip
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_source.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/roundtrip_test.cpp"
)
target_include_directories(x3d_assetimport_roundtrip PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support"
)
target_link_libraries(x3d_assetimport_roundtrip PRIVATE x3d_cpp::sdk x3d_cpp::authoring)
target_compile_features(x3d_assetimport_roundtrip PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_roundtrip COMMAND x3d_assetimport_roundtrip)

