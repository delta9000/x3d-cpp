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
