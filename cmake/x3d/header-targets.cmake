# cmake/x3d/header-targets.cmake — INTERFACE header layers for the generated nodes and hand-written runtime.

# Header usage requirements are a separate acyclic layer. Compiled libraries
# consume x3d_cpp_headers; the public x3d_cpp facade consumes both headers and
# compiled implementations. Keeping those directions separate lets the node
# runtime be either shared or static without a target dependency cycle.
add_library(x3d_cpp_headers INTERFACE)
add_library(x3d_cpp::headers ALIAS x3d_cpp_headers)
set_target_properties(x3d_cpp_headers PROPERTIES EXPORT_NAME headers)

add_library(x3d_cpp INTERFACE)
add_library(x3d_cpp::x3d_cpp ALIAS x3d_cpp)
target_link_libraries(x3d_cpp INTERFACE x3d_cpp_headers)

# Public headers include each other by bare name. This list drives both build
# and install include paths and the installed header set. Backend dependencies
# stay private and are excluded from the installed surface.
set(X3D_CPP_RUNTIME_PUBLIC_SUBDIRS codecs events math scene parse extract script sound hanim)

set(_x3d_cpp_interface_includes
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
foreach(_sub IN LISTS X3D_CPP_RUNTIME_PUBLIC_SUBDIRS)
    list(APPEND _x3d_cpp_interface_includes
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/${_sub}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/${_sub}>")
endforeach()
target_include_directories(x3d_cpp_headers INTERFACE ${_x3d_cpp_interface_includes})

# The generated code targets C++20 (designated initializers, etc.).
target_compile_features(x3d_cpp_headers INTERFACE cxx_std_20)
