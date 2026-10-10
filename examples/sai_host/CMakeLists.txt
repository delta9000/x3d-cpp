cmake_minimum_required(VERSION 3.24)
project(x3d_sai_cpu_host LANGUAGES CXX C)

# Installed consumption is the default. Source builds are deliberately explicit
# so a copied/relocated example never reaches into an adjacent checkout.
set(X3D_CPP_SOURCE_DIR "" CACHE PATH "Optional x3d-cpp source checkout")
set(X3D_SAI_SOURCE_DIR "" CACHE PATH "Optional x3d-sai source checkout")
if(X3D_CPP_SOURCE_DIR OR X3D_SAI_SOURCE_DIR)
  if(NOT EXISTS "${X3D_CPP_SOURCE_DIR}/CMakeLists.txt" OR
     NOT EXISTS "${X3D_SAI_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Source mode requires both X3D_CPP_SOURCE_DIR and X3D_SAI_SOURCE_DIR")
  endif()
  set(X3D_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(X3D_CPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(X3D_CPP_BUILD_SCRIPT OFF CACHE BOOL "" FORCE)
  set(X3D_CPP_BUILD_CGLTF OFF CACHE BOOL "" FORCE)
  set(X3D_CPP_ENABLE_DEV_TOOLING OFF CACHE BOOL "" FORCE)
  add_subdirectory("${X3D_SAI_SOURCE_DIR}" x3d-sai EXCLUDE_FROM_ALL)
  add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d-cpp EXCLUDE_FROM_ALL)
  add_subdirectory("${X3D_CPP_SOURCE_DIR}/adapters/sai_provider" adapter EXCLUDE_FROM_ALL)
else()
  find_package(x3d_cpp_sai CONFIG REQUIRED)
endif()

add_executable(sai_cpu_host main.cpp)
target_compile_features(sai_cpu_host PRIVATE cxx_std_20)
target_link_libraries(sai_cpu_host PRIVATE x3d_cpp::sai_provider x3d_cpp::x3d_cpp)
include(CTest)
if(BUILD_TESTING)
  add_test(NAME sai_cpu_host_recording COMMAND sai_cpu_host
    "${CMAKE_CURRENT_SOURCE_DIR}/recorded-workload.txt")
  set_tests_properties(sai_cpu_host_recording PROPERTIES TIMEOUT 30)
  add_test(NAME sai_cpu_host_rejects_malformed COMMAND "${CMAKE_COMMAND}"
    "-DHOST_EXECUTABLE=$<TARGET_FILE:sai_cpu_host>"
    "-DTEST_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/parser-fixtures"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/parser-rejections.cmake")
  set_tests_properties(sai_cpu_host_rejects_malformed PROPERTIES TIMEOUT 15)
endif()
