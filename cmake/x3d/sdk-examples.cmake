# cmake/x3d/sdk-examples.cmake — the x3d::sdk facade examples, built and run as ctests.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# T-SDK: the headless façade examples. Built + run as ctests so they cannot
# rot. Each links only x3d_cpp::sdk and uses only the x3d::sdk surface.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_EXAMPLES)
    foreach(_ex
        01_load_validate_convert
        02_extract_render_feed
        03_attach_behavior_tick)
        add_executable("x3d_example_${_ex}"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/${_ex}.cpp")
        target_link_libraries("x3d_example_${_ex}" PRIVATE x3d_cpp::sdk)
        add_test(NAME "x3d_example_${_ex}" COMMAND "x3d_example_${_ex}")
    endforeach()

    # -----------------------------------------------------------------------
    # Corpus smoke — drive the full pipeline over a whole archive of real
    # scenes (parse->context->extract->tick) and report categorized stats.
    # The binary takes a dir + flags; on-demand full sweep via `mise run corpus`.
    # The ctest runs a BOUNDED, deterministic subset and SKIPS (exit 0) if the
    # corpus dir is absent, so it stays portable.
    # -----------------------------------------------------------------------
    add_executable(x3d_corpus_sweep
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/corpus_sweep.cpp")
    target_link_libraries(x3d_corpus_sweep PRIVATE x3d_cpp::sdk)

    # Root of the X3D conformance archive for the corpus smoke. The archive is
    # not bundled (size + per-file Web3D attribution); point this at a local
    # checkout via -DX3D_CPP_CORPUS_DIR=... or the X3D_CORPUS_DIR env var. When
    # unset/absent the corpus_sweep exits 0 + "SKIPPED", so the test stays green.
    set(X3D_CPP_CORPUS_DIR "$ENV{X3D_CORPUS_DIR}"
        CACHE PATH "Root of the X3D conformance archive for the corpus smoke")
    add_test(NAME x3d_corpus_smoke
        COMMAND x3d_corpus_sweep "${X3D_CPP_CORPUS_DIR}"
                --limit 250 --min-success-ratio 0.85 --quiet)

    # -----------------------------------------------------------------------
    # Phase 0: the DIFFERENTIAL AUDIT harness. One clean binary that runs the
    # oracle-free invariant + round-trip-fidelity checks over the archive and
    # emits a machine-clusterable JSONL of findings. Check logic lives in the
    # shared header tools/corpus_audit.hpp (also driven by the selftest below).
    # The ctest runs a BOUNDED subset and SKIPS (exit 0) if the corpus is absent.
    # -----------------------------------------------------------------------
    add_executable(x3d_corpus_audit
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/corpus_audit.cpp")
    target_include_directories(x3d_corpus_audit
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    target_link_libraries(x3d_corpus_audit PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_corpus_audit_smoke
        COMMAND x3d_corpus_audit "${X3D_CPP_CORPUS_DIR}"
                --limit 250 --roundtrip-limit 250 --quiet
                --out "${CMAKE_CURRENT_BINARY_DIR}/corpus_audit_findings.jsonl")

    # TDD selftest for the audit engine: a clean scene yields ZERO findings
    # across all 3 encodings; a deliberately mismatched / unparseable case is
    # detected. Links the full node set so it can build a real scene graph.
    add_executable(x3d_corpus_audit_selftest
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/corpus_audit_selftest.cpp")
    target_include_directories(x3d_corpus_audit_selftest
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    target_link_libraries(x3d_corpus_audit_selftest PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_corpus_audit_selftest COMMAND x3d_corpus_audit_selftest)

    # CLI-level regression test for the corpus tools (arg-parsing edge cases,
    # empty dirs, exit-code consistency, usage completeness).
    add_test(NAME x3d_corpus_tools_cli_test
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/corpus_tools_cli_test.sh"
                "$<TARGET_FILE:x3d_corpus_sweep>"
                "$<TARGET_FILE:x3d_corpus_audit>")
endif()
