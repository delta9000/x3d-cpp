cmake_minimum_required(VERSION 3.24)
project(x3d_cpp_sai_hosted VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Explicit companion opt-in: normal x3d-cpp builds do not discover x3d-sai.
if(NOT TARGET x3d_cpp::x3d_cpp)
  find_package(x3d_cpp CONFIG REQUIRED)
endif()
if(NOT TARGET x3d::sai_hosted)
  find_package(x3d_sai CONFIG REQUIRED)
endif()
add_library(x3d_cpp_sai_hosted STATIC src/hosted.cpp)
add_library(x3d_cpp::sai_hosted ALIAS x3d_cpp_sai_hosted)
set_target_properties(x3d_cpp_sai_hosted PROPERTIES
  EXPORT_NAME sai_hosted POSITION_INDEPENDENT_CODE ON)
target_compile_features(x3d_cpp_sai_hosted PUBLIC cxx_std_20)
target_include_directories(x3d_cpp_sai_hosted PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
# Real domain runtime only: no SAI reference kernel, metadata, render window or
# rendering backend. The public header itself exposes only portable SAI values.
target_link_libraries(x3d_cpp_sai_hosted PUBLIC x3d::sai_hosted
  PRIVATE x3d_cpp::x3d_cpp)

option(X3D_CPP_SAI_HOSTED_BUILD_TESTS "Build the native hosted companion tests" ${PROJECT_IS_TOP_LEVEL})
if(X3D_CPP_SAI_HOSTED_BUILD_TESTS)
  enable_testing()
  add_executable(x3d_cpp_sai_hosted_tests tests/hosted_native_test.cpp)
  target_link_libraries(x3d_cpp_sai_hosted_tests PRIVATE x3d_cpp::sai_hosted x3d_cpp::x3d_cpp)
  add_test(NAME sai_hosted_native_runtime COMMAND x3d_cpp_sai_hosted_tests)
  # Optional TEST-ONLY cross-backend equality. The adapter/native-only executable
  # do not acquire reference-kernel or generated-SAI metadata dependencies.
  if(TARGET x3d::sai_reference_hosted)
    add_executable(x3d_cpp_sai_hosted_parity_tests tests/hosted_parity_test.cpp)
    target_link_libraries(x3d_cpp_sai_hosted_parity_tests PRIVATE
      x3d_cpp::sai_hosted x3d::sai_reference_hosted)
    add_test(NAME sai_hosted_cross_backend_parity COMMAND x3d_cpp_sai_hosted_parity_tests)
  endif()
endif()

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp_sai_hosted")
install(TARGETS x3d_cpp_sai_hosted EXPORT x3d_cpp_sai_hostedTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp")
install(EXPORT x3d_cpp_sai_hostedTargets FILE x3d_cpp_sai_hostedTargets.cmake
  NAMESPACE x3d_cpp:: DESTINATION "${_package_dir}")
configure_package_config_file(cmake/x3d_cpp_sai_hostedConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_sai_hostedConfig.cmake"
  INSTALL_DESTINATION "${_package_dir}")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_sai_hostedConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}" COMPATIBILITY ExactVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_sai_hostedConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_sai_hostedConfigVersion.cmake"
  DESTINATION "${_package_dir}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../NOTICE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/x3d_cpp_sai_hosted")
