cmake_minimum_required(VERSION 3.24)
project(x3d_cpp_sai VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Explicit opt-in companion package. Ordinary x3d-cpp builds do not include it,
# find x3d-sai, fetch another repository, or acquire a new dependency.
if(NOT TARGET x3d_cpp::x3d_cpp)
    find_package(x3d_cpp CONFIG REQUIRED)
endif()
if(NOT TARGET x3d::sai)
    find_package(x3d_sai CONFIG REQUIRED)
endif()

add_library(x3d_cpp_sai_provider STATIC src/sai_provider.cpp)
add_library(x3d_cpp::sai_provider ALIAS x3d_cpp_sai_provider)
set_target_properties(x3d_cpp_sai_provider PROPERTIES
    EXPORT_NAME sai_provider POSITION_INDEPENDENT_CODE ON)
target_compile_features(x3d_cpp_sai_provider PUBLIC cxx_std_20)
target_include_directories(x3d_cpp_sai_provider PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
target_link_libraries(x3d_cpp_sai_provider PUBLIC x3d::sai
    PRIVATE x3d_cpp::x3d_cpp)

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp_sai")
install(TARGETS x3d_cpp_sai_provider EXPORT x3d_cpp_saiTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp")
install(EXPORT x3d_cpp_saiTargets FILE x3d_cpp_saiTargets.cmake
    NAMESPACE x3d_cpp:: DESTINATION "${_package_dir}")
configure_package_config_file(cmake/x3d_cpp_saiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_saiConfig.cmake"
    INSTALL_DESTINATION "${_package_dir}")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_saiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY ExactVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_saiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_saiConfigVersion.cmake"
    DESTINATION "${_package_dir}")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../NOTICE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/x3d_cpp_sai")
