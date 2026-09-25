# cmake/x3d/header-targets.cmake — INTERFACE header layers for the generated nodes and hand-written runtime.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Header usage requirements are a separate acyclic layer. Compiled libraries
# consume x3d_cpp_headers; the public x3d_cpp facade consumes both headers and
# compiled implementations. Keeping those directions separate lets the node
# runtime be either shared or static without a target dependency cycle.
# ---------------------------------------------------------------------------
add_library(x3d_cpp_headers INTERFACE)
add_library(x3d_cpp::headers ALIAS x3d_cpp_headers)
set_target_properties(x3d_cpp_headers PROPERTIES EXPORT_NAME headers)

add_library(x3d_cpp INTERFACE)
add_library(x3d_cpp::x3d_cpp ALIAS x3d_cpp)
target_link_libraries(x3d_cpp INTERFACE x3d_cpp_headers)

# Generated headers live in generated_cpp_bindings/. They #include each other
# by bare name (e.g. #include "X3DNode.hpp"), so that directory must be on the
# include path. The hand-written runtime/ document model (X3DDocument, Scene,
# Route, PROTO, IMPORT/EXPORT) is header-only too and includes the generated
# nodes by bare name, so it joins the same include interface. Use BUILD_INTERFACE
# for in-tree consumers and INSTALL_INTERFACE for installed consumers.
# The hand-written runtime/ subdirs that make up the PUBLIC SDK surface — every
# directory x3d/sdk.hpp transitively includes by bare name. This ONE list drives
# the build-interface include dirs, the install-interface include dirs, AND the
# install() rules further down, so the three can never drift apart.
#
# Internal reference backends are deliberately absent: runtime/io (curl/s3/stb/
# wuffs/freetype/...), runtime/physics (Jolt), and runtime/ext (external geometry).
# They implement seams the embedder supplies as std::function callbacks, are not
# part of the v1 façade, and pull vendored third-party deps — so they are neither
# on the public include path nor installed. The façade (include/x3d/sdk.hpp) is
# the one, well-trod entry point; see docs/sdk/README.md.
set(X3D_CPP_RUNTIME_PUBLIC_SUBDIRS codecs events math scene parse extract script sound)

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
