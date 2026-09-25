# cmake/x3d/consumers.cmake — out-of-SDK example consumers (PoC renderer, CPU rasterizer, SVG, asset import).
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# PoC OpenGL renderer — conditional, OFF by default. Added LAST so the
# x3d_cpp::x3d_cpp INTERFACE target it links already exists. When OFF this is a
# no-op and the default build/golden/ctest path is byte-for-byte unchanged.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_POC)
    add_subdirectory(examples/poc_renderer)
endif()

# ---------------------------------------------------------------------------
# Headless CPU rasterizer — conditional, OFF by default. Dependency-free; when
# OFF this is a no-op and the default build/golden/ctest path is unchanged.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_CPURASTER)
    enable_testing() # so the rasterizer's ctests register even when BUILD_TESTS is OFF.
    add_subdirectory(examples/cpu_raster)
endif()

# ---------------------------------------------------------------------------
# Headless SVG projector — conditional, OFF by default. Façade-only and
# dependency-free; when OFF this is a no-op and the default path is unchanged.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_X3D2SVG)
    enable_testing() # so the example's ctests register even when BUILD_TESTS is OFF.
    add_subdirectory(examples/x3d2svg)
endif()

# ---------------------------------------------------------------------------
# Asset-import consumer — conditional, OFF by default. See
# examples/asset_import/CMakeLists.txt for the authoring footprint smoke and
# the ImportScene IR tests. Task 11 finalizes the full subdirectory.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_ASSET_IMPORT)
    enable_testing() # register ctests even when BUILD_TESTS is OFF.
    add_subdirectory(examples/asset_import)
endif()
