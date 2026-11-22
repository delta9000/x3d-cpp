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
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/script/vendor/duktape>")
    set_target_properties(x3d_duktape PROPERTIES EXPORT_NAME duktape)
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
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/script>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/script>")
    set_target_properties(x3d_ecmascript_backend PROPERTIES EXPORT_NAME ecmascript)
    add_library(x3d_cpp::ecmascript ALIAS x3d_ecmascript_backend)

    # Installed as x3d_cpp::ecmascript so a find_package(x3d_cpp) embedder can
    # run Script nodes. EcmaScriptBackend.hpp includes
    # "vendor/duktape/duktape.h", so the two Duktape headers install at that
    # path: the one vendor directory allowed in the include tree (see the leak
    # gate in scripts/verify_install_embed.sh). install.cmake leaves
    # EcmaScriptBackend.hpp out of the generic script/ install, so a build
    # without the backend ships no header it cannot compile.
    install(TARGETS x3d_duktape x3d_ecmascript_backend
        EXPORT x3d_cppTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/EcmaScriptBackend.hpp"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/script")
    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duk_config.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/script/vendor/duktape")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/DUKTAPE_LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/x3d_cpp")
endif()
