# cmake/x3d/import.cmake — optional mesh-import backends and the import tests.
#
# x3d_cpp::import (IR, emit, texture plan, material GLSL, cgltf glTF backend)
# is always built (cmake/x3d/libraries.cmake). The Assimp and USD backends
# are source-tree libraries that are not installed: they bring a system or
# fetched third-party dependency the installed SDK does not carry.

option(X3D_CPP_BUILD_ASSIMP "Build the Assimp mesh-import backend (OBJ, FBX, Collada, ...; needs a system Assimp)" OFF)
option(X3D_CPP_BUILD_USD "Build the USD mesh-import backend (fetches tinyusdz and applies runtime/import/usd/tinyusdz.patch)" OFF)

set(_x3d_import_src "${CMAKE_CURRENT_SOURCE_DIR}/runtime/import")

if(X3D_CPP_BUILD_ASSIMP)
    find_package(assimp CONFIG QUIET)
    if(NOT assimp_FOUND)
        message(FATAL_ERROR
            "X3D_CPP_BUILD_ASSIMP=ON but no Assimp CMake package was found "
            "(install libassimp-dev / assimp, or set assimp_DIR).")
    endif()
    add_library(x3d_import_assimp STATIC "${_x3d_import_src}/assimp/AssimpSource.cpp")
    target_link_libraries(x3d_import_assimp PUBLIC x3d_cpp::import PRIVATE assimp::assimp)
    target_include_directories(x3d_import_assimp PUBLIC "${_x3d_import_src}/assimp")
endif()

if(X3D_CPP_BUILD_USD)
    include(FetchContent)
    FetchContent_Declare(tinyusdz
        GIT_REPOSITORY https://github.com/lighttransport/tinyusdz.git
        GIT_TAG 11a2d361a9b6a0c4bf8918027233858d80ed17c5)
    FetchContent_GetProperties(tinyusdz)
    if(NOT tinyusdz_POPULATED)
        FetchContent_Populate(tinyusdz)
        # The vendored patch adds USD composition (references, payloads,
        # inherits, variants) on top of the pinned commit, reads the full
        # UsdLuxLightAPI inputs (color, intensity, exposure, ...) on every
        # light type, and lets Tydra read a SkelAnimation whose
        # blendShapeWeights carry only time samples.
        set(_x3d_tinyusdz_patch "${_x3d_import_src}/usd/tinyusdz.patch")
        execute_process(
            COMMAND git apply --reverse --check "${_x3d_tinyusdz_patch}"
            WORKING_DIRECTORY "${tinyusdz_SOURCE_DIR}"
            RESULT_VARIABLE _x3d_tinyusdz_patched
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT _x3d_tinyusdz_patched EQUAL 0)
            execute_process(
                COMMAND git apply "${_x3d_tinyusdz_patch}"
                WORKING_DIRECTORY "${tinyusdz_SOURCE_DIR}"
                RESULT_VARIABLE _x3d_tinyusdz_apply)
            if(NOT _x3d_tinyusdz_apply EQUAL 0)
                message(FATAL_ERROR "could not apply ${_x3d_tinyusdz_patch} to ${tinyusdz_SOURCE_DIR}")
            endif()
        endif()
    endif()
    set(TINYUSDZ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(TINYUSDZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TINYUSDZ_NO_WERROR ON CACHE BOOL "" FORCE)
    add_subdirectory("${tinyusdz_SOURCE_DIR}" "${tinyusdz_BINARY_DIR}" EXCLUDE_FROM_ALL)
    # Third-party code: X3D_CPP_WERROR's directory-wide -Werror must not apply.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(tinyusdz_static PRIVATE -w)
    endif()

    add_library(x3d_import_usd STATIC "${_x3d_import_src}/usd/UsdSource.cpp")
    target_link_libraries(x3d_import_usd PUBLIC x3d_cpp::import PRIVATE tinyusdz_static)
    target_include_directories(x3d_import_usd PUBLIC "${_x3d_import_src}/usd")
    target_include_directories(x3d_import_usd PRIVATE "${tinyusdz_SOURCE_DIR}/src")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_import_usd PRIVATE -Wno-unused-parameter -Wno-deprecated-declarations)
    endif()
endif()

if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    set(_x3d_import_fixtures "${_x3d_import_src}/tests/fixtures")
    add_executable(x3d_import_tests
        "${_x3d_import_src}/tests/registry_test.cpp"
        "${_x3d_import_src}/tests/emit_test.cpp"
        "${_x3d_import_src}/tests/texture_plan_test.cpp"
        "${_x3d_import_src}/tests/material_glsl_test.cpp"
        "${_x3d_import_src}/tests/cgltf_source_test.cpp"
        "${_x3d_import_src}/tests/roundtrip_test.cpp")
    target_link_libraries(x3d_import_tests PRIVATE x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_import_tests PRIVATE
        "${_x3d_import_src}"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    target_compile_definitions(x3d_import_tests PRIVATE
        "X3D_IMPORT_FIXTURES=\"${_x3d_import_fixtures}\""
        "X3D_REPO_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
    if(TARGET x3d_stb)
        target_link_libraries(x3d_import_tests PRIVATE x3d_stb)
        target_compile_definitions(x3d_import_tests PRIVATE X3D_IMPORT_HAVE_STB=1)
    endif()
    add_test(NAME x3d_import COMMAND x3d_import_tests)
    set_tests_properties(x3d_import PROPERTIES TIMEOUT 300)

    if(TARGET x3d_import_assimp)
        add_executable(x3d_import_assimp_test "${_x3d_import_src}/tests/assimp_source_test.cpp")
        target_link_libraries(x3d_import_assimp_test PRIVATE x3d_import_assimp x3d_cpp::sdk x3d_doctest_main)
        target_include_directories(x3d_import_assimp_test PRIVATE "${_x3d_import_src}")
        target_compile_definitions(x3d_import_assimp_test PRIVATE
            "X3D_IMPORT_FIXTURES=\"${_x3d_import_fixtures}\"")
        add_test(NAME x3d_import_assimp COMMAND x3d_import_assimp_test)
    endif()

    if(TARGET x3d_import_usd)
        add_executable(x3d_import_usd_test "${_x3d_import_src}/tests/usd_source_test.cpp")
        target_link_libraries(x3d_import_usd_test PRIVATE x3d_import_usd x3d_cpp::sdk x3d_doctest_main)
        target_include_directories(x3d_import_usd_test PRIVATE "${_x3d_import_src}")
        target_compile_definitions(x3d_import_usd_test PRIVATE
            "X3D_IMPORT_FIXTURES=\"${_x3d_import_fixtures}\""
            "X3D_TINYUSDZ_MODELS=\"${tinyusdz_SOURCE_DIR}/models\"")
        add_test(NAME x3d_import_usd COMMAND x3d_import_usd_test)
    endif()
endif()
