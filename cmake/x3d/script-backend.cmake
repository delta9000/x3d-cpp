# cmake/x3d/script-backend.cmake — the ECMAScript/Duktape ScriptEngine backend (vendored, ON by default).
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# ECMAScript/Duktape ScriptEngine backend (runtime/script/EcmaScriptBackend.*
# + vendored runtime/script/vendor/duktape/). The PRIMARY, production-facing
# backend for the language-agnostic ScriptEngine seam (runtime/script/
# ScriptEngine.hpp is CORE, header-only, JS-free, always present) — this is
# what makes Script nodes actually run, as opposed to being reported inert.
#
# Historically this only existed inside X3D_CPP_BUILD_TESTS, so the README's
# own primary build recipe (-DX3D_CPP_BUILD_TESTS=OFF) produced an `x3d`
# binary that could never execute Script nodes. Decoupled into its own option,
# mirroring the x3d_physics_jolt / x3d_quickjs isolation rule (option() -> an
# isolated static lib), except there is no FetchContent step: Duktape 2.7.0 is
# vendored in-repo (MIT license), so there is no network fetch and no extra
# dependency to acquire. ON by default — like X3D_CPP_BUILD_CGLTF — so the
# documented build recipe keeps working without a README change; turn it OFF
# only if you deliberately want a Script-free build.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_SCRIPT "Build the ECMAScript/Duktape ScriptEngine backend (vendored, MIT; ON default)" ON)

if(X3D_CPP_BUILD_SCRIPT)
    # Duktape 2.7.0 is vendored at runtime/script/vendor/duktape/ (MIT license).
    # It is a plain C source file; compile it as C (not C++) so it picks up its
    # own duk_config.h without any C++ name-mangling. Isolated into a static lib
    # (x3d_duktape) so the rest of the build is completely unaffected — nothing
    # except the ECMAScript backend target links it.
    add_library(x3d_duktape STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c")
    # Compile as C (it must not be parsed as C++).
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c"
        PROPERTIES LANGUAGE C)
    target_include_directories(x3d_duktape PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")
    # Keep Duktape warnings from polluting the main build.
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_duktape PRIVATE -w)
    endif()

    # EcmaScriptBackend: the Duktape-backed ScriptEngine implementation.
    add_library(x3d_ecmascript_backend STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/EcmaScriptBackend.cpp")
    target_link_libraries(x3d_ecmascript_backend PUBLIC
        x3d_cpp::x3d_cpp
        x3d_duktape)
    target_include_directories(x3d_ecmascript_backend PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")
endif()
