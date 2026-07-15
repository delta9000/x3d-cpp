# Contract: adding x3d_cpp with add_subdirectory() must not mutate state that
# reaches OUTSIDE its own subtree.
#
# Scoping facts this fixture is built on (MEASURED, not assumed -- an earlier
# draft asserted the wrong thing and could never fail):
#
#   - add_compile_options()/add_link_options() apply to the current directory and
#     BELOW. They never reach the parent, so asserting on the parent's directory
#     COMPILE_OPTIONS would pass even with the bug present. NOT tested here.
#   - set(... CACHE ...) writes the SHARED cache and DOES reach the parent.
#   - set_property(GLOBAL ...) is global by definition.
#
# Those last two are what this fixture pins down.
cmake_minimum_required(VERSION 3.21)
project(x3d_dev_tooling_fixture LANGUAGES CXX)

# CONTROL. Created before the subproject, so the cache has not been written yet
# and its CXX_COMPILER_LAUNCHER is baked empty. It must stay empty whether or not
# the bug is present -- it exists to prove the fixture discriminates, not to
# witness the mutation.
add_library(consumer_before STATIC consumer.cpp)

# Consume x3d-cpp exactly as a downstream project would.
add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d_cpp_sub EXCLUDE_FROM_ALL)

# WITNESS. Created after the subproject, so this is the target that actually
# inherits any cache-level launcher x3d_cpp wrote. This is the one that goes red.
add_library(consumer_after STATIC consumer.cpp)

set(_failures "")

# 1. The cached launcher. This is the confirmed cross-project mutation:
#    set(CMAKE_CXX_COMPILER_LAUNCHER ... CACHE STRING) makes ccache the default
#    for every target in the consumer's tree, not just x3d_cpp's.
if(CMAKE_CXX_COMPILER_LAUNCHER)
  list(APPEND _failures
       "x3d_cpp set CMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER} in the cache as a subproject; a dependency must not choose the consumer's launcher")
endif()
if(CMAKE_C_COMPILER_LAUNCHER)
  list(APPEND _failures
       "x3d_cpp set CMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER} in the cache as a subproject")
endif()

# 2. A parent-defined target must keep its own (empty) launcher. consumer_after
#    is the real witness; consumer_before is the control and must never trip.
foreach(_t consumer_before consumer_after)
  get_target_property(_launcher ${_t} CXX_COMPILER_LAUNCHER)
  if(_launcher)
    list(APPEND _failures
         "parent target ${_t} inherited CXX_COMPILER_LAUNCHER=${_launcher} from x3d_cpp")
  endif()
endforeach()

# 3. The global Ninja job pool caps the CONSUMER's build parallelism too.
get_property(_pools GLOBAL PROPERTY JOB_POOLS)
if(_pools MATCHES "x3d_compile")
  list(APPEND _failures
       "x3d_cpp defined the GLOBAL job pool '${_pools}' as a subproject; this bounds the consumer's own compiles")
endif()

if(_failures)
  string(REPLACE ";" "\n  - " _msg "${_failures}")
  message(FATAL_ERROR "dev-tooling isolation FAILED:\n  - ${_msg}")
endif()
message(STATUS "dev-tooling isolation OK: subproject reached nothing outside its own subtree")
