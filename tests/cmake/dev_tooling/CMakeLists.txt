# Contract: adding x3d_cpp with add_subdirectory() must not mutate state that
# reaches outside its own subtree.
#
# Directory compile/link options do not reach a parent. Shared cache writes
# and global properties do, so this fixture checks those two forms of state.
cmake_minimum_required(VERSION 3.21)
project(x3d_dev_tooling_fixture LANGUAGES CXX)

# The host owns shared-library and generic backend cache choices.
set(_host_options BUILD_SHARED_LIBS ENABLE_INSTALL GENERATE_DEBUG_SYMBOLS)
foreach(_option IN LISTS _host_options)
  set(${_option} ON CACHE BOOL "Host option")
endforeach()

# A target created before the subproject keeps an empty launcher.
add_library(consumer_before STATIC consumer.cpp)

# Consume x3d-cpp exactly as a downstream project would.
add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d_cpp_sub EXCLUDE_FROM_ALL)

# A later parent target must not inherit a subproject launcher.
add_library(consumer_after STATIC consumer.cpp)

set(_failures "")
foreach(_option IN LISTS _host_options)
  get_property(_cached CACHE ${_option} PROPERTY VALUE)
  if(NOT _cached OR NOT ${_option})
    list(APPEND _failures "x3d_cpp overwrote the parent's ${_option}=ON")
  endif()
endforeach()

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

# 3. The global Ninja job pool caps the consumer's build parallelism too.
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
