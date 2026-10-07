cmake_minimum_required(VERSION 3.24)
project(x3d_sai_provider_proof LANGUAGES CXX C)
include(CTest)
option(SAI_PROVIDER_INSTALLED "Consume relocated installed packages" OFF)
if(SAI_PROVIDER_INSTALLED)
    find_package(x3d_cpp_sai CONFIG REQUIRED)
else()
    get_filename_component(_default_cpp_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
    set(X3D_CPP_SOURCE_DIR "${_default_cpp_root}" CACHE PATH "x3d-cpp source checkout")
    set(X3D_SAI_SOURCE_DIR "" CACHE PATH "Independent x3d-sai source checkout")
    if(NOT EXISTS "${X3D_SAI_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "Set X3D_SAI_SOURCE_DIR to the provider-enabled x3d-sai checkout")
    endif()
    set(X3D_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(X3D_CPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(X3D_CPP_BUILD_SCRIPT OFF CACHE BOOL "" FORCE)
    set(X3D_CPP_BUILD_CGLTF OFF CACHE BOOL "" FORCE)
    set(X3D_CPP_ENABLE_DEV_TOOLING OFF CACHE BOOL "" FORCE)
    add_subdirectory("${X3D_SAI_SOURCE_DIR}" x3d-sai EXCLUDE_FROM_ALL)
    add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d-cpp EXCLUDE_FROM_ALL)
    add_subdirectory("${X3D_CPP_SOURCE_DIR}/adapters/sai_provider" adapter EXCLUDE_FROM_ALL)
endif()

add_executable(sai_provider_native native.cpp)
target_link_libraries(sai_provider_native PRIVATE
    x3d_cpp::sai_provider x3d_cpp::x3d_cpp x3d::sai_provider_fixture)
add_executable(sai_provider_reference reference.cpp)
target_link_libraries(sai_provider_reference PRIVATE
    x3d::sai_reference_provider x3d::sai_provider_fixture)
add_executable(sai_provider_parity parity.cpp)
target_link_libraries(sai_provider_parity PRIVATE
    x3d_cpp::sai_provider x3d::sai_reference_provider x3d::sai_provider_fixture)
foreach(kind IN ITEMS native reference parity)
    add_test(NAME sai_provider_${kind} COMMAND sai_provider_${kind})
    set_tests_properties(sai_provider_${kind} PROPERTIES TIMEOUT 15)
endforeach()
# The native-only binary is deliberately separate from parity, which links both.
# Its exact link command is checked by the integration script as an independence
# assertion: no reference kernel/metadata library may satisfy its symbols.
add_custom_target(sai_provider_all DEPENDS
    sai_provider_native sai_provider_reference sai_provider_parity)

# Native diagnostic/extraction evidence is a separate extension, never part of
# the normative provider contract. The source is copied for relocated testing.
if(SAI_PROVIDER_INSTALLED)
  set(_evidence "${CMAKE_CURRENT_LIST_DIR}/native_runtime_test.cpp")
else()
  set(_evidence "${X3D_CPP_SOURCE_DIR}/adapters/sai_provider/tests/native_runtime_test.cpp")
endif()
add_executable(sai_provider_native_runtime "${_evidence}")
target_link_libraries(sai_provider_native_runtime PRIVATE
  x3d_cpp::sai_provider x3d_cpp::x3d_cpp x3d::sai_provider_fixture)
add_test(NAME sai_provider_native_runtime COMMAND sai_provider_native_runtime)
set_tests_properties(sai_provider_native_runtime PROPERTIES TIMEOUT 30)
add_dependencies(sai_provider_all sai_provider_native_runtime)
