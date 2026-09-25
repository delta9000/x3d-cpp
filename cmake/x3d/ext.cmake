# cmake/x3d/ext.cmake — the opt-in x3d::runtime::ext extensions (X3D_CPP_BUILD_EXT)
# and their tests.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Extension quarantine — x3d_cpp_ext (INTERFACE, header-only).
# Lives in runtime/ext/; namespace x3d::runtime::ext.
# One-way dep: ext→core (includes PackedMesh, Aabb, …). Core NEVER includes ext.
# Default OFF — no TU in runtime/ext/ is ever compiled or linked in the
# standard build path. Tests registered ONLY inside this block, so the default
# ctest suite is unaffected. Build with: -DX3D_CPP_BUILD_EXT=ON
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_EXT)
    add_library(x3d_cpp_ext INTERFACE)
    add_library(x3d_cpp::ext ALIAS x3d_cpp_ext)
    target_include_directories(x3d_cpp_ext INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/codecs>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_compile_features(x3d_cpp_ext INTERFACE cxx_std_20)
    # Ext code may #include PackedMesh.hpp, Aabb.hpp, etc. from core.
    target_link_libraries(x3d_cpp_ext INTERFACE x3d_cpp::x3d_cpp)

    # Ext tests — registered inside the quarantine block only.
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()

        add_executable(x3d_ext_stl_reader
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/stl_reader_test.cpp")
        target_link_libraries(x3d_ext_stl_reader PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_stl_reader COMMAND x3d_ext_stl_reader)

        # STL writer round-trip test: writeStlBinary → parseStlBinary (bit-exact).
        # Uses StlReader (ext-gated) to validate the writer output.
        add_executable(x3d_stl_write_test
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/stl_write_test.cpp")
        target_include_directories(x3d_stl_write_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
        target_link_libraries(x3d_stl_write_test PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_stl_write_test COMMAND x3d_stl_write_test)

        # Extract self-oracle: X3D → writeStlBinary → parseStlBinary → compare.
        # Proves extract path agrees with StlReader end-to-end.
        add_executable(x3d_extract_oracle_test
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/extract_oracle_test.cpp")
        target_include_directories(x3d_extract_oracle_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
        target_link_libraries(x3d_extract_oracle_test PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_extract_oracle_test
            COMMAND x3d_extract_oracle_test
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures/simple.x3d"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures/validate-clean.x3d")

        add_executable(x3d_ext_external_geometry_node
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_node_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_node PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_node COMMAND x3d_ext_external_geometry_node)

        add_executable(x3d_ext_external_geometry_roundtrip
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_roundtrip_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_roundtrip PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_roundtrip COMMAND x3d_ext_external_geometry_roundtrip)

        # Round 2 — lazy ExternalGeometry materialization end-to-end proof.
        add_executable(x3d_ext_external_geometry_e2e
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_e2e_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_e2e PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_e2e COMMAND x3d_ext_external_geometry_e2e)
    endif()
endif()
