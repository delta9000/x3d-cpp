cmake_minimum_required(VERSION 3.21)
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${CMAKE_CURRENT_LIST_DIR}" -B "${FIXTURE_BINARY_DIR}"
        -G "${FIXTURE_GENERATOR}"
        "-DX3D_CPP_SOURCE_DIR=${X3D_CPP_SOURCE_DIR}"
        "-DX3D_CPP_BUILD_EXAMPLES=${X3D_CPP_BUILD_EXAMPLES}"
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${FIXTURE_BINARY_DIR}"
        --show-only=json-v1
    OUTPUT_VARIABLE _json
    COMMAND_ERROR_IS_FATAL ANY)

# Audit the generated CTest inventory without requiring any compiled binaries.
# This also catches future tests silently omitted by label-filtered PR CI.
string(JSON _count LENGTH "${_json}" tests)
if(_count EQUAL 0)
    message(FATAL_ERROR "The selection fixture registered no tests")
endif()
math(EXPR _last "${_count} - 1")
foreach(_index RANGE ${_last})
    string(JSON _name GET "${_json}" tests ${_index} name)
    string(JSON _property_count LENGTH "${_json}" tests ${_index} properties)
    set(_selected FALSE)
    if(_property_count GREATER 0)
        math(EXPR _property_last "${_property_count} - 1")
        foreach(_property RANGE ${_property_last})
            string(JSON _key GET "${_json}" tests ${_index} properties ${_property} name)
            if(_key STREQUAL "LABELS")
                string(JSON _labels GET "${_json}" tests ${_index} properties ${_property} value)
                if(_labels MATCHES "\"(behavior|compile-contract)\"")
                    set(_selected TRUE)
                endif()
            endif()
        endforeach()
    endif()
    if(NOT _selected)
        message(FATAL_ERROR "${_name} is not selected by either PR CI suite")
    endif()
endforeach()
