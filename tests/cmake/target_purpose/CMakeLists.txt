cmake_minimum_required(VERSION 3.20)
project(x3d_target_purpose_fixture LANGUAGES CXX)

if(NOT DEFINED X3D_CPP_SOURCE_DIR)
    message(FATAL_ERROR "X3D_CPP_SOURCE_DIR is required")
endif()

list(APPEND CMAKE_MODULE_PATH "${X3D_CPP_SOURCE_DIR}/cmake")
include(X3DTargetPurpose)

add_library(classified INTERFACE)
add_library(classified_alias ALIAS classified)
add_library(external_dependency INTERFACE IMPORTED)

if(TEST_CASE STREQUAL "duplicate")
    x3d_set_target_purpose(classified production)
    x3d_set_target_purpose(classified behavior)
elseif(TEST_CASE STREQUAL "invalid")
    x3d_set_target_purpose(classified accidental)
elseif(TEST_CASE STREQUAL "missing-suite-target")
    x3d_set_target_purpose(classified production)
    x3d_register_test_target(does_not_exist SUITES behavior)
else()
    x3d_set_target_purpose(classified production)
    x3d_register_test_target(classified SUITES behavior sanitizer)

    x3d_get_targets_by_purpose(_production_targets production)
    if(NOT _production_targets STREQUAL "classified")
        message(FATAL_ERROR
            "unexpected production target list: '${_production_targets}'")
    endif()

    x3d_get_test_targets(_behavior_targets behavior)
    if(NOT _behavior_targets STREQUAL "classified")
        message(FATAL_ERROR
            "unexpected behavior target list: '${_behavior_targets}'")
    endif()

    if(TEST_CASE STREQUAL "unclassified")
        add_library(unclassified INTERFACE)
        add_library(unclassified_second INTERFACE)
    endif()
endif()

x3d_validate_target_purposes()
