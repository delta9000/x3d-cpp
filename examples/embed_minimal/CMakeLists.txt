cmake_minimum_required(VERSION 3.21)
project(x3d_embed_minimal LANGUAGES CXX)

find_package(x3d_cpp CONFIG REQUIRED)

add_executable(x3d_embed_minimal main.cpp)
target_link_libraries(x3d_embed_minimal PRIVATE x3d_cpp::sdk)

add_executable(x3d_embed_authoring authoring.cpp)
target_link_libraries(x3d_embed_authoring PRIVATE x3d_cpp::authoring)

add_executable(x3d_embed_native_host native_host.cpp)
target_link_libraries(x3d_embed_native_host PRIVATE x3d_cpp::sdk)

option(X3D_EMBED_EXT "Consume the installed opt-in extension layer" OFF)
if(X3D_EMBED_EXT)
    add_executable(x3d_embed_ext ext.cpp)
    target_link_libraries(x3d_embed_ext PRIVATE x3d_cpp::sdk x3d_cpp::ext)
endif()
