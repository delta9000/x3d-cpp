# cmake/x3d/libraries.cmake — compiled node + runtime libraries and the x3d::sdk facade.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Compiled node library (C1): the generated *.cpp (per-node out-of-line
# reflection/validate definitions + the X3DNodeFactory registry) are compiled
# ONCE here instead of re-instantiated in every consumer TU. Consumers link
# this transitively via x3d_cpp::x3d_cpp.
# ---------------------------------------------------------------------------
file(GLOB X3D_CPP_NODE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/nodes/*.cpp")

# generator.py also emits a standalone value-smoke program beside the node
# definitions. Its main() was inert in the old static archive because no link
# requested that object; it is not a production node definition and must never
# enter either library form.
set(_x3d_generated_test_source
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/nodes/test.cpp")
list(REMOVE_ITEM X3D_CPP_NODE_SOURCES "${_x3d_generated_test_source}")

option(X3D_CPP_SHARED_NODES
    "Build the generated node runtime as one shared library (OFF for a static archive)"
    ON)
if(X3D_CPP_SHARED_NODES)
    set(_x3d_node_library_type SHARED)
    message(STATUS "x3d_cpp: generated node runtime = shared")
else()
    set(_x3d_node_library_type STATIC)
    message(STATUS "x3d_cpp: generated node runtime = static")
endif()

add_library(x3d_cpp_nodes ${_x3d_node_library_type} ${X3D_CPP_NODE_SOURCES})
add_library(x3d_cpp::nodes ALIAS x3d_cpp_nodes)

# Unity-build the generated node TUs: batching the per-node .cpp into grouped
# TUs eliminates the repeated framework-header re-parse that dominated this
# library's compile — ~2x faster cold build (measured: 30s -> 14s, 16 cores),
# at ~0.4 GB peak RSS per default 8-node TU (well within CI runner limits; the
# pre-C1-split OOM scare was the inline thunks, since removed). Consumers that
# add_subdirectory() this project inherit the speedup.
#
set_target_properties(x3d_cpp_nodes PROPERTIES
    EXPORT_NAME nodes
    UNITY_BUILD ON)
if(X3D_CPP_SHARED_NODES)
    set_target_properties(x3d_cpp_nodes PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${X3D_CPP_SOVERSION}"
        WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
target_link_libraries(x3d_cpp_nodes PUBLIC x3d_cpp_headers)
target_compile_features(x3d_cpp_nodes PUBLIC cxx_std_20)

# ---------------------------------------------------------------------------
# Compiled hand-written runtime layers. They follow the generated node
# library's linkage mode so an implementation graph is either entirely shared
# (the default) or entirely static (X3D_CPP_SHARED_NODES=OFF). Mixing modes can
# absorb the same generated symbols into multiple DSOs, so it is deliberately
# not exposed as a supported configuration.
# ---------------------------------------------------------------------------
add_library(x3d_cpp_authoring_runtime ${_x3d_node_library_type}
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/FieldValueIO.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/ChildOrder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/XmlWriter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/JsonWriter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/VrmlWriter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/CanonicalXmlWriter.cpp")
add_library(x3d_cpp::authoring_runtime ALIAS x3d_cpp_authoring_runtime)
set_target_properties(x3d_cpp_authoring_runtime PROPERTIES
    EXPORT_NAME authoring_runtime)
target_link_libraries(x3d_cpp_authoring_runtime PUBLIC x3d_cpp::nodes)
target_compile_features(x3d_cpp_authoring_runtime PUBLIC cxx_std_20)

add_library(x3d_cpp_runtime ${_x3d_node_library_type}
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/X3DParse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/NodeBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/ClassicVrmlReader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/JsonReader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/Vrml97Reader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/XmlReaderAdapter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/MeshBuilder.cpp")
add_library(x3d_cpp::runtime ALIAS x3d_cpp_runtime)
set_target_properties(x3d_cpp_runtime PROPERTIES EXPORT_NAME runtime)
target_link_libraries(x3d_cpp_runtime PUBLIC x3d_cpp::authoring_runtime)
target_compile_features(x3d_cpp_runtime PUBLIC cxx_std_20)

if(X3D_CPP_SHARED_NODES)
    if(APPLE)
        set(_x3d_runtime_install_rpath "@loader_path")
    elseif(UNIX)
        set(_x3d_runtime_install_rpath "$ORIGIN")
    endif()
    foreach(_runtime_target IN ITEMS
            x3d_cpp_authoring_runtime
            x3d_cpp_runtime)
        set_target_properties(${_runtime_target} PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${X3D_CPP_SOVERSION}"
            WINDOWS_EXPORT_ALL_SYMBOLS ON)
        # Each installed DSO resolves the next implementation layer beside
        # itself. Executable RUNPATH is not inherited for indirect dependencies.
        if(DEFINED _x3d_runtime_install_rpath)
            set_property(TARGET ${_runtime_target} PROPERTY
                INSTALL_RPATH "${_x3d_runtime_install_rpath}")
        endif()
    endforeach()
    unset(_x3d_runtime_install_rpath)
endif()

# Consumers of the complete interface receive all compiled definitions through
# the full runtime's public dependency chain.
target_link_libraries(x3d_cpp INTERFACE x3d_cpp::runtime)

# ── Authoring slim target: node reflection + writers + range-validate only ──
# INTERFACE facade over the compiled node library plus the authoring-only
# headers (document model, range-validate, codec writers). Deliberately omits
# the parse/execution/extract include dirs so consumers cannot reach those APIs.
add_library(x3d_cpp_authoring INTERFACE)
add_library(x3d_cpp::authoring ALIAS x3d_cpp_authoring)
set_target_properties(x3d_cpp_authoring PROPERTIES EXPORT_NAME authoring)
target_link_libraries(x3d_cpp_authoring INTERFACE
    x3d_cpp::authoring_runtime)
target_include_directories(x3d_cpp_authoring INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
target_compile_features(x3d_cpp_authoring INTERFACE cxx_std_20)
install(TARGETS x3d_cpp_authoring EXPORT x3d_cppTargets)

# ---------------------------------------------------------------------------
# T-SDK: the curated embedder façade. A thin INTERFACE target that adds the
# include/ path (so consumers write #include "x3d/sdk.hpp") on top of the full
# x3d_cpp interface. No new compiled TU — sdk.hpp only re-exports.
# ---------------------------------------------------------------------------
add_library(x3d_cpp_sdk INTERFACE)
add_library(x3d_cpp::sdk ALIAS x3d_cpp_sdk)
set_target_properties(x3d_cpp_sdk PROPERTIES EXPORT_NAME sdk)
target_include_directories(x3d_cpp_sdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
target_link_libraries(x3d_cpp_sdk INTERFACE x3d_cpp::x3d_cpp)
