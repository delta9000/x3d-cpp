# cmake/x3d/cli.cmake — the x3d CLI binary, the QuickJS swap-test, and the CLI differential / canonicalize gates.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# x3d CLI — thin `x3d` binary over the x3d::sdk façade.
# Subcommands: convert (Task A); validate slots in next (Task B).
# Always built alongside the other tools/ binaries in the default preset.
# ---------------------------------------------------------------------------
add_executable(x3d_cli
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d_cli.cpp")
set_target_properties(x3d_cli PROPERTIES OUTPUT_NAME "x3d")
target_include_directories(x3d_cli PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_cli PRIVATE x3d_cpp::sdk)
# `x3d sim` drives the behavior runtime; link the ECMAScript backend so Script
# nodes actually run when it is available (it lives behind X3D_CPP_BUILD_SCRIPT,
# ON by default, independent of X3D_CPP_BUILD_TESTS). When absent (explicitly
# turned OFF), sim degrades cleanly: scripts are inert (reported, not a
# failure). The compile define gates the optional ScriptSystem include.
if(TARGET x3d_ecmascript_backend)
    target_link_libraries(x3d_cli PRIVATE x3d_ecmascript_backend)
    target_compile_definitions(x3d_cli PRIVATE X3D_SIM_HAVE_SCRIPT=1)
endif()
# When the Jolt backend is built (X3D_CPP_BUILD_PHYSICS=ON) link it so `x3d sim`
# simulates §37 rigid bodies; X3D_HAVE_PHYSICS comes transitively from the lib.
# OFF (default) => attachPhysics is the documented no-op; x3d_cli is unchanged.
if(TARGET x3d_physics_jolt)
    target_link_libraries(x3d_cli PRIVATE x3d_physics_jolt)
endif()

# CLI test: arg parsing, convert round-trips, help, error paths, + `sim`
# (golden traces, --watch, --json, determinism, malformed-input robustness),
# plus a gallery/smoke scene profile gate (validate exits 0 on every
# first-party showcase scene under examples/cpu_raster/assets/).
# Passes the binary path, in-repo fixture dir, and the goldens dir to the harness.
# The 4th arg signals whether x3d_cli was built with the Jolt physics backend so
# the harness runs (or skips) its §37 sim-physics assertions accordingly. The
# 5th arg is the repo root, so the harness can locate the gallery/smoke scenes.
if(TARGET x3d_physics_jolt)
    set(_x3d_cli_physics 1)
else()
    set(_x3d_cli_physics 0)
endif()
add_test(NAME x3d_cli_test
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/x3d_cli_test.sh"
            "$<TARGET_FILE:x3d_cli>"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/goldens"
            "${_x3d_cli_physics}"
            "${CMAKE_CURRENT_SOURCE_DIR}")

# sim behavioral self-oracle (C++): the runtime actually interpolating
# (translation.x ~= 5 at t~=0.5) + ProximitySensor enter/exit on a viewer path +
# determinism (two runs byte-identical). Links the full SDK; the fixtures dir is
# passed as argv[1]. Gated by X3D_CPP_BUILD_TESTS like the rest of the test
# suite, so a tests-off "production" build does not compile this test binary.
if(X3D_CPP_BUILD_TESTS)
    add_executable(x3d_sim_behavior_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/sim_behavior_test.cpp")
    target_include_directories(x3d_sim_behavior_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
    target_link_libraries(x3d_sim_behavior_test PRIVATE x3d_cpp::sdk)
    if(TARGET x3d_ecmascript_backend)
        target_link_libraries(x3d_sim_behavior_test PRIVATE x3d_ecmascript_backend)
        target_compile_definitions(x3d_sim_behavior_test PRIVATE X3D_SIM_HAVE_SCRIPT=1)
    endif()
    add_test(NAME x3d_sim_behavior_test
        COMMAND x3d_sim_behavior_test
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures")
endif()

# ---------------------------------------------------------------------------
# U3 ScriptEngine GENERICITY PROOF — the backend swap-test (x3d_quickjs_swap).
# Drives the SAME authored Script fixtures through the full runtime under BOTH
# the Duktape EcmaScriptBackend and the QuickJsBackend over an identical tick
# schedule, and asserts an IDENTICAL observable trace (author-field values +
# ROUTE-target values). Built ONLY when X3D_CPP_BUILD_QUICKJS=ON (when ON,
# x3d_ecmascript_backend — the always-built Duktape backend — is also present,
# so both engines link). Placed here, after both targets are defined, so the
# single TU links both backends.
# ---------------------------------------------------------------------------
if(TARGET x3d_quickjs AND TARGET x3d_ecmascript_backend AND X3D_CPP_BUILD_TESTS)
    add_executable(x3d_quickjs_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/quickjs_swap_test.cpp")
    target_link_libraries(x3d_quickjs_swap PRIVATE
        x3d_quickjs
        x3d_ecmascript_backend)
    add_test(NAME x3d_quickjs_swap
        COMMAND x3d_quickjs_swap
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/data")
endif()

# ---------------------------------------------------------------------------
# x3d-cli differential gate:
#   x3d_scene_equiv_test  — unit tests for sceneEquivalent() (built under
#                           X3D_CPP_BUILD_TESTS, like the rest of the suite)
#   x3d_cli_gate          — Java-free differential gate (validate-diff +
#                           convert-roundtrip vs X3DJSAIL golden verdicts)
#
# Both targets include tools/x3d-cli/ so they can see scene_equiv.hpp and
# cli_gate.cpp without polluting the SDK include path.
# The self-contained tests remain in the default build. The external-corpus
# gates exist as explicit targets, but are excluded from `all` and from CTest;
# their mise tasks build them before use.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_TESTS)
    add_executable(x3d_scene_equiv_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/scene_equiv_test.cpp")
    target_include_directories(x3d_scene_equiv_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
    target_link_libraries(x3d_scene_equiv_test PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_scene_equiv_test COMMAND x3d_scene_equiv_test)

    # Regression for the gate's Inline/EXTERNPROTO baseUrl asymmetry (BUG-1):
    # the reparse must use the same baseUrl as the source parse so relative
    # externals expand symmetrically. Self-contained (writes its own temp
    # fixtures), so it is a real ctest (unlike x3d_cli_gate which needs the
    # external corpus).
    add_executable(x3d_gate_baseurl_roundtrip_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/gate_baseurl_roundtrip_test.cpp")
    target_include_directories(x3d_gate_baseurl_roundtrip_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
    target_link_libraries(x3d_gate_baseurl_roundtrip_test PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_gate_baseurl_roundtrip_test
             COMMAND x3d_gate_baseurl_roundtrip_test)
endif()

add_executable(x3d_cli_gate EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/cli_gate.cpp")
target_include_directories(x3d_cli_gate PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_cli_gate PRIVATE x3d_cpp::sdk)
# Note: x3d_cli_gate is NOT registered as a ctest — it requires an external
# corpus and committed golden files. Run via: mise run cli-gate

# ---------------------------------------------------------------------------
# x3d canonicalize — X3DC14N canonical form gate:
#   x3d_canonicalize_unit_test  — unit tests for CanonicalXmlWriter (ctest,
#                                 built under X3D_CPP_BUILD_TESTS)
#   x3d_canon_gate              — tiered differential gate vs X3DJSAIL golden
#                                 fixtures; NOT a ctest (needs external corpus).
#                                 Run via: mise run canon-gate
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_TESTS)
    add_executable(x3d_canonicalize_unit_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/canonicalize_unit_test.cpp")
    target_include_directories(x3d_canonicalize_unit_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs")
    target_link_libraries(x3d_canonicalize_unit_test PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_canonicalize_unit_test COMMAND x3d_canonicalize_unit_test)
endif()

add_executable(x3d_canon_gate EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/canon_gate.cpp")
target_include_directories(x3d_canon_gate PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_canon_gate PRIVATE x3d_cpp::sdk)
# Note: x3d_canon_gate is NOT registered as a ctest — requires external corpus.
# Run via: mise run canon-gate
