# cmake/x3d/consumers.cmake — out-of-SDK example consumers (PoC renderer, CPU rasterizer, SVG, asset import).

if(X3D_CPP_BUILD_POC)
    add_subdirectory(examples/poc_renderer)
endif()

if(X3D_CPP_BUILD_CPURASTER)
    enable_testing() # so the rasterizer's ctests register even when BUILD_TESTS is OFF.
    add_subdirectory(examples/cpu_raster)
endif()

if(X3D_CPP_BUILD_X3D2SVG)
    enable_testing() # so the example's ctests register even when BUILD_TESTS is OFF.
    add_subdirectory(examples/x3d2svg)
endif()

if(X3D_CPP_BUILD_ASSET_IMPORT)
    enable_testing() # register ctests even when BUILD_TESTS is OFF.
    add_subdirectory(examples/asset_import)
endif()
