# cmake/x3d/target-purposes.cmake — target purpose inventory (primary purpose per repository-owned target).
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Target purpose inventory. Primary purpose answers why each repository-owned
# target exists; test-suite membership is a separate property added in the
# aggregate-target phase. Optional names are kept in the inventory even when
# their feature flag is off so enabling a backend cannot create an unclassified
# target later.
# ---------------------------------------------------------------------------
function(_x3d_classify_present_targets purpose)
    foreach(_target IN LISTS ARGN)
        if(TARGET "${_target}")
            x3d_set_target_purpose("${_target}" "${purpose}")
        endif()
    endforeach()
endfunction()

_x3d_classify_present_targets(production
    x3d_cpp_headers
    x3d_cpp
    x3d_cpp_nodes
    x3d_cpp_authoring_runtime
    x3d_cpp_runtime
    x3d_cpp_authoring
    x3d_cpp_sdk
    x3d_ecmascript_backend
    x3d_cli)

_x3d_classify_present_targets(compile-contract
    x3d_cpp_all_headers)

_x3d_classify_present_targets(behavior
    x3d_codec_roundtrip_audit
    x3d_xml_script_field
    x3d_xml_composed_shader
    x3d_vrml_script_field
    x3d_json_script_field
    x3d_contact_reporter
    x3d_physics_contact_wiring
    x3d_inertia_massprops
    x3d_contact_response
    x3d_event_scene_bridge
    x3d_sai_context
    x3d_script_system
    x3d_ecmascript_backend_test
    x3d_ecmascript_corpus_smoke_test
    x3d_script_author_runtime
    x3d_script_corpus_e2e
    x3d_parse_reader
    x3d_classic_vrml_reader
    x3d_proto_front_door
    x3d_proto_expand_audit
    x3d_inline_roundtrip
    x3d_inline_routes
    x3d_inline_cycle
    x3d_inflate_bomb
    x3d_path_confine
    x3d_inline_containment_cycle
    x3d_vrml97_reader
    x3d_json_reader
    x3d_sdk_facade
    x3d_authoring_link_contract
    x3d_sound_system
    x3d_corpus_sweep
    x3d_corpus_audit
    x3d_corpus_audit_selftest
    x3d_sim_behavior_test
    x3d_scene_equiv_test
    x3d_gate_baseurl_roundtrip_test
    x3d_canonicalize_unit_test
    x3d_geometry_scene_tests
    x3d_namespace_taxonomy
    x3d_codecs_tests
    x3d_parse_tests
    x3d_extract_tests
    x3d_events_tests
    x3d_fileresolver_test)

_x3d_classify_present_targets(opt-in
    x3d_physics_jolt
    x3d_physics_jolt_test
    x3d_physics_system_test
    x3d_quickjs
    x3d_quickjs_backend_test
    x3d_curl
    x3d_assetresolver_backend_a_test
    x3d_s3
    x3d_assetresolver_backend_b_test
    x3d_assetresolver_swap
    x3d_stb
    x3d_wuffs
    x3d_stbtt
    x3d_plmpeg
    x3d_theora
    x3d_freetype
    x3d_text_tests
    x3d_texture_tests
    x3d_movie_tests
    x3d_example_01_load_validate_convert
    x3d_example_02_extract_render_feed
    x3d_example_03_attach_behavior_tick
    x3d_quickjs_swap
    x3d_cli_gate
    x3d_canon_gate
    x3d_cpp_ext
    x3d_ext_stl_reader
    x3d_stl_write_test
    x3d_extract_oracle_test
    x3d_ext_external_geometry_node
    x3d_ext_external_geometry_roundtrip
    x3d_ext_external_geometry_e2e
    x3d_miniaudio
    x3d_sound_swaptest
    x3d_parse_fuzz)

_x3d_classify_present_targets(internal
    x3d_duktape
    x3d_s3_testsupport
    x3d_doctest_main)

# Test-suite membership is deliberately independent of primary target purpose:
# x3d_cli is production software and still belongs in the behavior/sanitizer
# build graph, while behavior test executables naturally belong in both.
if(X3D_CPP_BUILD_TESTS)
    x3d_get_targets_by_purpose(_x3d_behavior_targets behavior)
    foreach(_target IN LISTS _x3d_behavior_targets)
        x3d_register_test_target("${_target}" SUITES behavior sanitizer)
    endforeach()
    if(TARGET x3d_cli)
        x3d_register_test_target(x3d_cli SUITES behavior sanitizer)
    endif()

    x3d_get_targets_by_purpose(_x3d_contract_targets compile-contract)
    foreach(_target IN LISTS _x3d_contract_targets)
        x3d_register_test_target("${_target}" SUITES compile-contract)
    endforeach()

    x3d_get_test_targets(_x3d_behavior_suite_targets behavior)
    add_custom_target(x3d_behavior_tests)
    add_dependencies(x3d_behavior_tests ${_x3d_behavior_suite_targets})
    x3d_set_target_purpose(x3d_behavior_tests internal)

    x3d_get_test_targets(_x3d_sanitizer_suite_targets sanitizer)
    add_custom_target(x3d_sanitizer_tests)
    add_dependencies(x3d_sanitizer_tests ${_x3d_sanitizer_suite_targets})
    x3d_set_target_purpose(x3d_sanitizer_tests internal)

    x3d_get_test_targets(_x3d_contract_suite_targets compile-contract)
    add_custom_target(x3d_compile_contracts)
    add_dependencies(x3d_compile_contracts ${_x3d_contract_suite_targets})
    x3d_set_target_purpose(x3d_compile_contracts internal)

    # Keep execution selection as explicit as build selection. Some test names
    # differ from their owning targets, and shell-driven tests have no owning
    # executable, so CTest labels are assigned from the public test names.
    set(_x3d_behavior_tests
        x3d_codec_roundtrip_audit
        x3d_xml_script_field
        x3d_xml_composed_shader
        x3d_vrml_script_field
        x3d_json_script_field
        x3d_contact_reporter
        x3d_physics_contact_wiring
        x3d_inertia_massprops
        x3d_contact_response
        x3d_event_scene_bridge
        x3d_sai_context
        x3d_script_system
        x3d_ecmascript_backend
        x3d_ecmascript_corpus_smoke
        x3d_script_author_runtime
        x3d_script_corpus_e2e
        x3d_parse_reader
        x3d_classic_vrml_reader
        x3d_proto_front_door
        x3d_proto_expand_audit
        x3d_inline_roundtrip
        x3d_inline_routes
        x3d_inline_cycle
        x3d_inflate_bomb
        x3d_path_confine
        x3d_inline_containment_cycle
        x3d_vrml97_reader
        x3d_json_reader
        x3d_sdk_facade
        x3d_authoring_link_contract
        x3d_sound_system
        x3d_corpus_smoke
        x3d_corpus_audit_smoke
        x3d_corpus_audit_selftest
        x3d_corpus_tools_cli_test
        x3d_cli_test
        x3d_sim_behavior_test
        x3d_scene_equiv_test
        x3d_gate_baseurl_roundtrip_test
        x3d_canonicalize_unit_test
        x3d_geometry_scene
        x3d_namespace_taxonomy
        x3d_codecs_tests
        x3d_parse_tests
        x3d_extract_tests
        x3d_events_tests
        x3d_fileresolver_test)
    if(NOT X3D_CPP_SAN)
        list(APPEND _x3d_behavior_tests x3d_install_embed_smoke)
    endif()
    set_tests_properties(${_x3d_behavior_tests} PROPERTIES LABELS behavior)

    set(_x3d_compile_contract_tests x3d_cpp_all_headers)
    if(X3D_CPP_PER_HEADER_CHECKS)
        list(APPEND _x3d_compile_contract_tests x3d_header_isolation)
    endif()
    set_tests_properties(
        ${_x3d_compile_contract_tests} PROPERTIES LABELS compile-contract)
endif()

x3d_validate_target_purposes()
