# PROJECT_IS_TOP_LEVEL requires CMake 3.21.
cmake_minimum_required(VERSION 3.21)

project(x3d_cpp
    VERSION 0.1.0
    DESCRIPTION "Headless, renderer-agnostic X3D domain-runtime SDK: C++ node bindings generated from the X3D Unified Object Model, plus a compiled runtime"
    LANGUAGES CXX C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/X3DTargetPurpose.cmake")

# Before 1.0, sonames track major.minor so the loader enforces minor-version
# ABI separation. From 1.0, the major version is the ABI boundary.
# Configure-time package checks alone do not constrain the dynamic loader.
# See CONTRIBUTING.md for the compatibility policy.
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}")
endif()

# The build is split by area under cmake/x3d/. Order matters: later fragments
# use options, targets and variables defined by earlier ones.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/dev-tooling.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/header-targets.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/compiler-flags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/libraries.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/install.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/options.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/backends.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/script-backend.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/tests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/sdk-examples.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/cli.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/consumers.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/ext.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/doctest-suites.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/fuzz.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/target-purposes.cmake")
