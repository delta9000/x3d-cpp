# ---------------------------------------------------------------------------
# Asset-import authoring footprint smoke. Builds a tiny consumer against ONLY
# x3d_cpp::authoring to prove the slim surface links and serializes a scene.
# Task 11 finalizes the full asset-import subdirectory.
# ---------------------------------------------------------------------------
add_executable(x3d_authoring_smoke
    "${CMAKE_CURRENT_SOURCE_DIR}/footprint/authoring_smoke.cpp")
target_link_libraries(x3d_authoring_smoke PRIVATE x3d_cpp::authoring)
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
