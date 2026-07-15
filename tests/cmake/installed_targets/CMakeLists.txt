# Contract: every installed imported target must exist after find_package().
#
# examples/embed_minimal only links x3d_cpp::sdk and x3d_cpp::authoring, so a
# broken EXPORT_NAME on any other installed target would not be caught -- the
# in-tree ALIAS would still resolve while the installed name silently differed.
#
# The two groups below are NOT the same promise. PUBLIC names are documented
# entry points: renaming one is a breaking change. IMPLEMENTATION names exist so
# the export set is closed over the dependency graph (a consumer linking
# x3d_cpp::sdk needs them to resolve); they are asserted because their absence
# breaks find_package(), NOT because they are promised as stable entry points.
# Do not cite this file as evidence that the implementation names are API.
cmake_minimum_required(VERSION 3.21)
project(x3d_installed_targets_contract LANGUAGES CXX)

find_package(x3d_cpp CONFIG REQUIRED)

# Documented entry points. Changing these is a breaking change.
set(_public
    x3d_cpp::sdk
    x3d_cpp::authoring
    x3d_cpp::x3d_cpp)

# Required for the export set to resolve. Restructurable; not promised.
set(_implementation
    x3d_cpp::headers
    x3d_cpp::nodes
    x3d_cpp::authoring_runtime
    x3d_cpp::runtime)

set(_missing_public "")
foreach(_t IN LISTS _public)
  if(NOT TARGET ${_t})
    list(APPEND _missing_public ${_t})
  endif()
endforeach()

set(_missing_impl "")
foreach(_t IN LISTS _implementation)
  if(NOT TARGET ${_t})
    list(APPEND _missing_impl ${_t})
  endif()
endforeach()

if(_missing_public)
  string(REPLACE ";" "\n  - " _msg "${_missing_public}")
  message(FATAL_ERROR
    "installed target contract FAILED -- missing PUBLIC entry points:\n  - ${_msg}\n"
    "These are documented names. Each installed target needs an explicit "
    "EXPORT_NAME matching its in-tree ALIAS, or the documented name will not "
    "exist downstream.")
endif()

if(_missing_impl)
  string(REPLACE ";" "\n  - " _msg "${_missing_impl}")
  message(FATAL_ERROR
    "installed target contract FAILED -- missing implementation targets:\n  - ${_msg}\n"
    "These are not public API, but the export set must be closed over them or "
    "find_package() cannot resolve the public entry points that depend on them.")
endif()

message(STATUS "installed target contract OK: 3 public + 4 implementation targets present")
