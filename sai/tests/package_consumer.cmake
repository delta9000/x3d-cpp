# Run ordinary applications through both CMake entry points. Reuse the public
# examples so their documented authoring workflows also prove the SDK boundary.
set(work "${SAI_BINARY_DIR}/package-consumer")
# Keep compiler caches, but recreate the installation so missing install rules
# cannot be hidden by files left over from a previous run.
file(REMOVE_RECURSE "${work}/original-prefix" "${work}/relocated-prefix")
file(MAKE_DIRECTORY "${work}/consumer")
file(COPY "${SAI_SOURCE_DIR}/tests/package_consumer/CMakeLists.txt"
    "${SAI_SOURCE_DIR}/tests/package_consumer/shared.cpp"
    "${SAI_SOURCE_DIR}/tests/package_consumer/shared_main.cpp"
    "${SAI_SOURCE_DIR}/tests/provider_reference_test.cpp"
    "${SAI_SOURCE_DIR}/tests/provider_schema_test.cpp"
    "${SAI_SOURCE_DIR}/examples/author_inspect.cpp"
    "${SAI_SOURCE_DIR}/examples/generated_author_inspect.cpp"
    "${SAI_SOURCE_DIR}/examples/provider_scene.cpp"
    DESTINATION "${work}/consumer")

function(run)
    execute_process(COMMAND ${ARGV} COMMAND_ERROR_IS_FATAL ANY)
endfunction()

run("${CMAKE_COMMAND}" --install "${SAI_BINARY_DIR}" --config "${SAI_CONFIG}"
    --prefix "${work}/original-prefix")
# A relocated prefix must work without the original installation path.
file(RENAME "${work}/original-prefix" "${work}/relocated-prefix")
foreach(private_path IN ITEMS x3d/core x3d/nodes x3d_sai)
    if(EXISTS "${work}/relocated-prefix/include/${private_path}")
        message(FATAL_ERROR "SAI implementation headers were installed: ${private_path}")
    endif()
endforeach()
file(GLOB_RECURSE configs "${work}/relocated-prefix/*.cmake")
foreach(config IN LISTS configs)
    file(READ "${config}" contents)
    foreach(path IN ITEMS "${SAI_SOURCE_DIR}" "${SAI_BINARY_DIR}")
        string(FIND "${contents}" "${path}" found)
        if(NOT found EQUAL -1)
            message(FATAL_ERROR "Installed CMake config leaks a checkout path: ${config}")
        endif()
    endforeach()
endforeach()

set(build_parallelism 3)
if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL} AND NOT "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}" STREQUAL "")
    set(build_parallelism "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}")
endif()
foreach(mode IN ITEMS installed embedded)
    set(arguments "-DCMAKE_PREFIX_PATH=${work}/relocated-prefix")
    if(mode STREQUAL "embedded")
        list(APPEND arguments "-DSAI_SOURCE_DIR=${SAI_SOURCE_DIR}")
    endif()
    run("${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/${mode}"
        -G "${SAI_GENERATOR}" "-DCMAKE_BUILD_TYPE=${SAI_CONFIG}"
        "-DCMAKE_CXX_COMPILER=${SAI_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS=${SAI_CXX_FLAGS}"
        "-DCMAKE_EXE_LINKER_FLAGS=${SAI_EXE_LINKER_FLAGS}"
        -DBUILD_TESTING=ON -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF
        -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF ${arguments})
    run("${CMAKE_COMMAND}" --build "${work}/${mode}" --config "${SAI_CONFIG}"
        --parallel "${build_parallelism}")
    run("${CMAKE_CTEST_COMMAND}" --test-dir "${work}/${mode}"
        --build-config "${SAI_CONFIG}" --output-on-failure)
endforeach()
