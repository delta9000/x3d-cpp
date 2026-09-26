# cmake/x3d/doctest-suites.cmake — the grouped doctest binaries (math/scene, codecs,
# parse, extract, events) and the namespace-taxonomy lock.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# --- PROTO: grouped doctest binary (math + scene tests, was 22 separate exes) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_geometry_scene_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/aabb_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_inverse_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/vec_math_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/intersect_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/geometry_bounds_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/getfield_typecheck_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/dirty_tracker_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/view_dependent_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/transform_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/transform_system_hanim_cadpart_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_shared_subgraph_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_cycle_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/cycle_breaker_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/walker_cycle_guard_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/walker_budget_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/pick_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/pick_index_equivalence_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/pick_index_perf_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/interface_registry_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/scene_extractor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/poc_triangle_asset_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_stack_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bind_time_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_stack_audit_test.cpp")
    target_link_libraries(x3d_geometry_scene_tests PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_geometry_scene_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    # poc_triangle_asset_test needs its fixture path (was a per-target define).
    target_compile_definitions(x3d_geometry_scene_tests PRIVATE
        "X3D_POC_TRIANGLE_ASSET=\"${CMAKE_CURRENT_SOURCE_DIR}/examples/poc_renderer/assets/triangle.x3d\"")
    add_test(NAME x3d_geometry_scene COMMAND x3d_geometry_scene_tests)
    set_tests_properties(x3d_geometry_scene PROPERTIES TIMEOUT 120)
endif()

# --- Namespace taxonomy lock (ADR-0039): pin x3d::core / x3d::nodes placement ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_namespace_taxonomy
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/tests/namespace_taxonomy_test.cpp")
    target_link_libraries(x3d_namespace_taxonomy PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_namespace_taxonomy PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_namespace_taxonomy COMMAND x3d_namespace_taxonomy)
endif()

# --- doctest grouped binary: codecs tests (19 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_codecs_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/vrml_mf_bracket_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_instance_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_body_defscope_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/enum_quote_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_proto_capture_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_decl_body_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_deep_is_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_is_json_vrml_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_writer_parity_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_appinfo_json_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_extern_url_json_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_nested_instance_placement_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/nested_protoinstance_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/initializeonly_read_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/codec_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/fval_extended_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/sfimage_overflow_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_depth_guard_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/script_cdata_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/uom_type_pin_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/hanim_container_order_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/codec_string_hardening_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/version_header_test.cpp")
    target_link_libraries(x3d_codecs_tests PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_codecs_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_codecs_tests COMMAND x3d_codecs_tests)
    set_tests_properties(x3d_codecs_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: parse tests (13 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_parse_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_clone_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/field_view_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_expand_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_nested_body_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/json_proto_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/vrml97_proto_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_carriers_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_expand_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/lenient_read_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/encoding_lex_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/range_warnings_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/range_validate_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/version_floor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/reader_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/parser_depth_guard_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/asset_proto_resolver_test.cpp")
    target_link_libraries(x3d_parse_tests PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_parse_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_parse_tests COMMAND x3d_parse_tests)
    set_tests_properties(x3d_parse_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: extract tests (35 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_extract_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_item_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_delta_perf_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_instancing_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_delta_contract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/runtime_session_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/material_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_mapping_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_orm_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b5_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_elevationgrid_winding_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b6_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_txc1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/text_layout_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/text_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/asset_resolver_b8_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_t7_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_t8_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_b2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_col2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_cad1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/light_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_m25_5_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_feed_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/packed_mesh_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_desc_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_item_geometry_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/external_geom_seam_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b7_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_ext002_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/shader_binding_plan_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/castshadow_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/nurbs_eval_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_nurbs_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scheme_router_test.cpp")
    target_link_libraries(x3d_extract_tests PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_extract_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_extract_tests COMMAND x3d_extract_tests)
    set_tests_properties(x3d_extract_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: events tests (34 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_events_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/dynamic_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/mem_safety_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_observer_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_alias_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2a_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2b_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2c_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/write_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2d_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/tick_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointer_state_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointing_sensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointing_sensor_skip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointer_arbitration_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/standard_runtime_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/nav_arbitration_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interactive_wiring_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/drag_math_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/key_state_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/navigation_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/nav_pointer_screen_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/viewpoint_offset_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/viewpoint_bind_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/animation_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interpolator_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/follower_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interpolator_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/event_utility_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/key_device_sensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/node_lifecycle_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/timesensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/timesensor_rtc_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_dynamic_route_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_author_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/loadsensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/slerp_normal_test.cpp")
    target_link_libraries(x3d_events_tests PRIVATE
        x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_events_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_events_tests COMMAND x3d_events_tests)
    set_tests_properties(x3d_events_tests PROPERTIES TIMEOUT 180)
endif()
