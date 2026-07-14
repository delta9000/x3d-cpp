cmake_minimum_required(VERSION 3.20)
project(x3d_embed_minimal LANGUAGES CXX)

find_package(x3d_cpp CONFIG REQUIRED)

add_executable(x3d_embed_minimal main.cpp)
target_link_libraries(x3d_embed_minimal PRIVATE x3d_cpp::sdk)

add_executable(x3d_embed_authoring authoring.cpp)
target_link_libraries(x3d_embed_authoring PRIVATE x3d_cpp::authoring)
