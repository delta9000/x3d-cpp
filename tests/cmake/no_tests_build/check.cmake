# Configure-only regression for the documented install build
# (docs/wiki/how-to/build-and-install.md): with X3D_CPP_BUILD_TESTS=OFF and the
# examples left at their default, no target may link x3d_doctest_main, which
# only exists when tests are on. A target that does fails to compile
# (doctest/doctest.h is not on its include path) only after a long build, so
# check the generated Ninja file instead.
cmake_minimum_required(VERSION 3.21)
if(NOT FIXTURE_GENERATOR MATCHES "Ninja")
    message(STATUS "no_tests_build: skipped, needs a Ninja generator")
    return()
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${X3D_CPP_SOURCE_DIR}" -B "${FIXTURE_BINARY_DIR}"
        -G "${FIXTURE_GENERATOR}"
        -DCMAKE_BUILD_TYPE=Release
        -DX3D_CPP_BUILD_TESTS=OFF
    COMMAND_ERROR_IS_FATAL ANY)
file(STRINGS "${FIXTURE_BINARY_DIR}/build.ninja" _hits REGEX "x3d_doctest_main")
if(_hits)
    list(GET _hits 0 _first)
    message(FATAL_ERROR
        "With X3D_CPP_BUILD_TESTS=OFF a target still links x3d_doctest_main; "
        "guard it with if(X3D_CPP_BUILD_TESTS). First hit:\n${_first}")
endif()
