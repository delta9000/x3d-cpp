# Configure the real project, then inspect exactly what the PR CI selects.
cmake_minimum_required(VERSION 3.21)
project(x3d_test_selection_fixture LANGUAGES CXX C)

set(X3D_CPP_BUILD_TESTS ON CACHE BOOL "" FORCE)
set(X3D_CPP_ENABLE_DEV_TOOLING OFF CACHE BOOL "" FORCE)
set(X3D_CPP_PER_HEADER_CHECKS OFF CACHE BOOL "" FORCE)
enable_testing()
add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d_cpp_sub EXCLUDE_FROM_ALL)

foreach(_suite IN ITEMS x3d_behavior_tests x3d_sanitizer_tests)
    get_target_property(_deps ${_suite} MANUALLY_ADDED_DEPENDENCIES)
    foreach(_target IN ITEMS
            x3d_example_01_load_validate_convert
            x3d_example_02_extract_render_feed
            x3d_example_03_attach_behavior_tick
            x3d_corpus_sweep x3d_corpus_audit x3d_corpus_audit_selftest)
        if(X3D_CPP_BUILD_EXAMPLES)
            if(NOT TARGET ${_target} OR NOT _target IN_LIST _deps)
                message(FATAL_ERROR "${_suite} does not build ${_target}")
            endif()
        elseif(TARGET ${_target} OR _target IN_LIST _deps)
            message(FATAL_ERROR "${_suite} depends on disabled example ${_target}")
        endif()
    endforeach()
endforeach()
