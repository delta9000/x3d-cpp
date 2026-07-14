include_guard(GLOBAL)

set(_X3D_VALID_TARGET_PURPOSES
    production
    behavior
    compile-contract
    opt-in
    internal)

set(_X3D_VALID_TEST_SUITES
    behavior
    sanitizer
    compile-contract)

function(_x3d_require_list_value value list_variable description)
    list(FIND ${list_variable} "${value}" _x3d_value_index)
    if(_x3d_value_index EQUAL -1)
        string(JOIN ", " _x3d_valid_values ${${list_variable}})
        message(FATAL_ERROR
            "invalid X3D ${description} '${value}'; expected one of: "
            "${_x3d_valid_values}")
    endif()
endfunction()

function(x3d_set_target_purpose target purpose)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR
            "cannot classify nonexistent repository-owned target '${target}'")
    endif()
    _x3d_require_list_value(
        "${purpose}" _X3D_VALID_TARGET_PURPOSES "target purpose")

    get_target_property(_x3d_existing_purpose "${target}" X3D_TARGET_PURPOSE)
    if(_x3d_existing_purpose)
        message(FATAL_ERROR
            "target '${target}' already has X3D_TARGET_PURPOSE "
            "'${_x3d_existing_purpose}'; duplicate classification as "
            "'${purpose}' is not allowed")
    endif()

    set_property(TARGET "${target}" PROPERTY X3D_TARGET_PURPOSE "${purpose}")
    set_property(GLOBAL APPEND PROPERTY "X3D_TARGETS_${purpose}" "${target}")
endfunction()

function(x3d_get_targets_by_purpose out_variable purpose)
    _x3d_require_list_value(
        "${purpose}" _X3D_VALID_TARGET_PURPOSES "target purpose")
    get_property(_x3d_targets GLOBAL PROPERTY "X3D_TARGETS_${purpose}")
    if(_x3d_targets)
        list(REMOVE_DUPLICATES _x3d_targets)
    endif()
    set("${out_variable}" "${_x3d_targets}" PARENT_SCOPE)
endfunction()

function(x3d_register_test_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR
            "cannot register nonexistent test target '${target}'")
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 _x3d_register "" "" "SUITES")
    if(_x3d_register_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "unrecognized arguments for x3d_register_test_target(${target}): "
            "${_x3d_register_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT _x3d_register_SUITES)
        message(FATAL_ERROR
            "x3d_register_test_target(${target}) requires at least one SUITES value")
    endif()

    foreach(_x3d_suite IN LISTS _x3d_register_SUITES)
        _x3d_require_list_value(
            "${_x3d_suite}" _X3D_VALID_TEST_SUITES "test suite")
        set_property(
            TARGET "${target}" APPEND PROPERTY X3D_TEST_SUITES "${_x3d_suite}")
        set_property(
            GLOBAL APPEND PROPERTY "X3D_TEST_TARGETS_${_x3d_suite}" "${target}")
    endforeach()

    get_target_property(_x3d_suites "${target}" X3D_TEST_SUITES)
    list(REMOVE_DUPLICATES _x3d_suites)
    set_property(TARGET "${target}" PROPERTY X3D_TEST_SUITES "${_x3d_suites}")
endfunction()

function(x3d_get_test_targets out_variable suite)
    _x3d_require_list_value(
        "${suite}" _X3D_VALID_TEST_SUITES "test suite")
    get_property(_x3d_targets GLOBAL PROPERTY "X3D_TEST_TARGETS_${suite}")
    if(_x3d_targets)
        list(REMOVE_DUPLICATES _x3d_targets)
    endif()
    set("${out_variable}" "${_x3d_targets}" PARENT_SCOPE)
endfunction()

function(x3d_validate_target_purposes)
    get_property(
        _x3d_root_targets
        DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTY BUILDSYSTEM_TARGETS)
    set(_x3d_unclassified)
    foreach(_x3d_target IN LISTS _x3d_root_targets)
        get_target_property(_x3d_imported "${_x3d_target}" IMPORTED)
        get_target_property(_x3d_aliased_target "${_x3d_target}" ALIASED_TARGET)
        if(_x3d_imported OR _x3d_aliased_target)
            continue()
        endif()

        get_target_property(
            _x3d_target_purpose "${_x3d_target}" X3D_TARGET_PURPOSE)
        if(NOT _x3d_target_purpose)
            list(APPEND _x3d_unclassified "${_x3d_target}")
        endif()
    endforeach()

    if(_x3d_unclassified)
        list(SORT _x3d_unclassified)
        set(_x3d_message "Every repository-owned target needs exactly one purpose:")
        foreach(_x3d_target IN LISTS _x3d_unclassified)
            string(APPEND _x3d_message
                "\n  repository-owned target '${_x3d_target}' has no "
                "X3D_TARGET_PURPOSE")
        endforeach()
        message(FATAL_ERROR "${_x3d_message}")
    endif()
endfunction()
