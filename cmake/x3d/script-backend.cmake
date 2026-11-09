# cmake/x3d/script-backend.cmake — the ECMAScript/Duktape ScriptEngine backend (vendored, ON by default).

# Build independently of tests so production CLI builds can execute Script nodes.
option(X3D_CPP_BUILD_SCRIPT "Build the ECMAScript/Duktape ScriptEngine backend (vendored, MIT; ON default)" ON)

if(X3D_CPP_BUILD_SCRIPT)
    # Duktape keeps extern "C" symbol names when compiled as C++.
    add_library(x3d_duktape STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c")
    # Compiled as C++ so Duktape raises script errors as C++ exceptions
    # (DUK_USE_CPP_EXCEPTIONS, set for C++ in duk_config.h) rather than longjmp:
    # an error thrown while the backend marshals values (a getter, toJSON, a
    # Proxy trap) then unwinds the backend's C++ frames with destructors run,
    # up to the duk_safe_call the backend wraps every engine entry in.
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c"
        PROPERTIES LANGUAGE CXX)
    target_include_directories(x3d_duktape PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")
    # Keep Duktape warnings from polluting the main build.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_duktape PRIVATE -w)
    elseif(MSVC)
        # /EHs, not the default /EHsc: the Duktape API is extern "C" but
        # throws, and /EHsc assumes extern "C" functions never do.
        target_compile_options(x3d_duktape PRIVATE /w /EHs)
    endif()

    # EcmaScriptBackend: the Duktape-backed ScriptEngine implementation.
    add_library(x3d_ecmascript_backend STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/EcmaScriptBackend.cpp")
    if(MSVC)
        target_compile_options(x3d_ecmascript_backend PRIVATE /EHs)
    endif()
    target_link_libraries(x3d_ecmascript_backend PUBLIC
        x3d_cpp::x3d_cpp
        x3d_duktape)
    target_include_directories(x3d_ecmascript_backend PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")
endif()
