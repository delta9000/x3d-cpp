# 3.21 is the floor because PROJECT_IS_TOP_LEVEL -- used to scope dev tooling and
# to default X3D_CPP_BUILD_TESTS / X3D_CPP_BUILD_EXAMPLES -- was added in 3.21.
# It was already in use while the floor said 3.20, where it silently evaluated
# empty and those options defaulted OFF.
cmake_minimum_required(VERSION 3.21)

project(x3d_cpp
    VERSION 0.1.0
    DESCRIPTION "Headless, renderer-agnostic X3D domain-runtime SDK: C++ node bindings generated from the X3D Unified Object Model, plus a compiled runtime"
    LANGUAGES CXX C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/X3DTargetPurpose.cmake")

# ---------------------------------------------------------------------------
# Shared-library soname. Pre-1.0 the ABI-breaking axis is the MINOR (SemVer says
# 0.y is unstable), so the soname must track major.minor: a plain
# ${PROJECT_VERSION_MAJOR} is 0 for every 0.x, which would let ld.so load a 0.2
# runtime into a 0.1 consumer that find_package() had just refused --
# write_basic_package_version_file(SameMinorVersion) governs configure time only,
# not the loader. From 1.0 the major IS the ABI axis and the conventional soname
# is correct, so this branch expires on its own at the version bump.
#
# The compatibility promise this encodes is INTENT, not proof: there is no ABI
# checker in CI yet. See CONTRIBUTING.md.
# ---------------------------------------------------------------------------
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}")
endif()

# ---------------------------------------------------------------------------
# The build is split by area under cmake/x3d/. Order matters: later fragments
# use options, targets and variables defined by earlier ones.
# ---------------------------------------------------------------------------
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
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/audio.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/fuzz.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d/target-purposes.cmake")
