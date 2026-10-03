cmake_minimum_required(VERSION 3.24)
project(x3d_sai VERSION 0.1.0 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)

set(_sai_generated "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings")

# Firewalled modern-C++ SAI proposal. The kernel uses the C++ standard runtime
# and platform threading; it has no dependency on x3d-cpp's generated objects.
add_library(x3d_sai_experimental STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/kernel.cpp")
add_library(x3d::sai ALIAS x3d_sai_experimental)
set_target_properties(x3d_sai_experimental PROPERTIES
    EXPORT_NAME sai POSITION_INDEPENDENT_CODE ON)
target_include_directories(x3d_sai_experimental PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_features(x3d_sai_experimental PUBLIC cxx_std_20)
target_link_libraries(x3d_sai_experimental PUBLIC Threads::Threads)

# Optional generated-schema bridge. Keeping this separate preserves the
# dependency-light semantic kernel while allowing consumers to opt into the UOM
# catalog.
add_library(x3d_sai_experimental_metadata STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/metadata.cpp"
    "${_sai_generated}/x3d/nodes/X3DSemanticMetadataRegistry.cpp")
add_library(x3d::sai_metadata ALIAS x3d_sai_experimental_metadata)
set_target_properties(x3d_sai_experimental_metadata PROPERTIES
    EXPORT_NAME sai_metadata POSITION_INDEPENDENT_CODE ON)
target_include_directories(x3d_sai_experimental_metadata PUBLIC
    "$<BUILD_INTERFACE:${_sai_generated}>")
target_link_libraries(x3d_sai_experimental_metadata PUBLIC x3d_sai_experimental)
target_compile_features(x3d_sai_experimental_metadata PUBLIC cxx_std_20)

# The same target names and usage requirements work in a source checkout and
# an installed SDK. Install only the public SAI headers; the catalog's generated
# reflection support stays inside the compiled metadata bridge.
set(_sai_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_sai")
install(TARGETS x3d_sai_experimental x3d_sai_experimental_metadata
    EXPORT x3d_saiTargets ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/x3d/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d"
    FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY "${_sai_generated}/x3d/sai/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d/sai"
    FILES_MATCHING PATTERN "*.hpp")
install(FILES third_party/tl/expected.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tl")
install(FILES LICENSE NOTICE DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/x3d_sai")
install(FILES third_party/tl/COPYING
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/x3d_sai"
    RENAME tl-expected-COPYING)
install(EXPORT x3d_saiTargets FILE x3d_saiTargets.cmake NAMESPACE x3d::
    DESTINATION "${_sai_package_dir}")
configure_package_config_file(cmake/x3d_saiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_saiConfig.cmake"
    INSTALL_DESTINATION "${_sai_package_dir}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/x3d_saiConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY ExactVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/x3d_saiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_saiConfigVersion.cmake"
    DESTINATION "${_sai_package_dir}")

# Embedding applications control their own testing. A standalone SDK build can
# also disable these targets with the standard BUILD_TESTING option.
if(NOT PROJECT_IS_TOP_LEVEL OR NOT BUILD_TESTING)
    return()
endif()

add_library(x3d_sai_doctest_main STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/doctest/doctest_main.cpp")
target_include_directories(x3d_sai_doctest_main PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_compile_features(x3d_sai_doctest_main PUBLIC cxx_std_20)

add_executable(x3d_sai_experimental_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_kernel_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/authoring_validation_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit_validation_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/scene_metadata_test.cpp")
target_link_libraries(x3d_sai_experimental_tests PRIVATE
    x3d_sai_experimental_metadata x3d_sai_doctest_main)
target_compile_features(x3d_sai_experimental_tests PRIVATE cxx_std_20)

# CTest names below are load-bearing: scripts/sai_conformance.py parses literal
# add_test(NAME ...) entries out of this file to cross-link registered services
# against their falsification tests. Do not rename them.
add_test(NAME x3d_sai_experimental COMMAND x3d_sai_experimental_tests)
add_test(NAME sai_id_shared_node COMMAND x3d_sai_experimental_tests
    "--test-case=one shared node has one identity and two occurrences")
add_test(NAME sai_id_occurrence_edge COMMAND x3d_sai_experimental_tests
    "--test-case=one shared node has one identity and two occurrences")
add_test(NAME sai_own_domain COMMAND x3d_sai_experimental_tests
    "--test-case=a browser rejects an execution context from another browser")
add_test(NAME sai_field_one_descriptor COMMAND x3d_sai_experimental_tests
    "--test-case=dynamic discovery and typed fields are substitutable")
add_test(NAME sai_generated_metadata COMMAND x3d_sai_experimental_tests
    "--test-case=generated metadata authors without runtime node identity")
add_test(NAME sai_generated_metadata_drift COMMAND x3d_sai_experimental_tests
    "--test-case=generated metadata adapter has exhaustive ordered descriptor parity")
add_test(NAME sai_field_complete_vocabulary COMMAND x3d_sai_experimental_tests
    "--test-case=the owning value vocabulary preserves every non-node ISO kind")
add_test(NAME sai_generated_metadata_executable COMMAND x3d_sai_experimental_tests
    "--test-case=every generated descriptor adapts into the executable vocabulary")
add_test(NAME sai_generated_metadata_lexical COMMAND x3d_sai_experimental_tests
    "--test-case=schema lexical defaults reject image truncation and preserve strings")
add_test(NAME sai_field_value_boundaries COMMAND x3d_sai_experimental_tests
    "--test-case=numeric bits and large owning images survive the semantic boundary")
add_test(NAME sai_field_equal_intent COMMAND x3d_sai_experimental_tests
    "--test-case=dynamic discovery and typed fields are substitutable")
add_test(NAME sai_mf_size_read COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_set COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_insert COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_erase COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_append COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_clear COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_replace COMMAND x3d_sai_experimental_tests
    "--test-case=a dynamic multi-field editor stages the complete indexed service")
add_test(NAME sai_mf_typed_dynamic COMMAND x3d_sai_experimental_tests
    "--test-case=typed and dynamic multi-field editors publish equal traces")
add_test(NAME sai_mf_revision_bound COMMAND x3d_sai_experimental_tests
    "--test-case=multi-field editors are revision-bound capabilities")
add_test(NAME sai_mf_node_authority COMMAND x3d_sai_experimental_tests
    "--test-case=MFNode editors validate the complete handle range before staging")
add_test(NAME sai_mf_event_trace COMMAND x3d_sai_experimental_tests
    "--test-case=typed and dynamic MF events preserve the complete owning payload")
add_test(NAME sai_field_access_state_machine COMMAND x3d_sai_experimental_tests
    "--test-case=field access is a lifecycle rule rather than a setter shape")
add_test(NAME sai_field_readable COMMAND x3d_sai_experimental_tests
    "--test-case=field capability queries predict every lifecycle intent")
add_test(NAME sai_field_writable COMMAND x3d_sai_experimental_tests
    "--test-case=field capability queries predict every lifecycle intent")
add_test(NAME sai_field_units_once COMMAND x3d_sai_experimental_tests
    "--test-case=authored unit boundaries convert exactly once around canonical state")
add_test(NAME sai_field_derived_units COMMAND x3d_sai_experimental_tests
    "--test-case=*derived*unit*")
add_test(NAME sai_field_unit_write_range COMMAND x3d_sai_experimental_tests
    "--test-case=authored unit writes reject finite payloads outside field range")
add_test(NAME sai_field_unit_event_range COMMAND x3d_sai_experimental_tests
    "--test-case=unit conversion failure cancels earlier events in the same batch")
add_test(NAME sai_field_unit_read_range COMMAND x3d_sai_experimental_tests
    "--test-case=inverse unit reads reject finite canonical values outside field range")
add_test(NAME sai_field_pure_read COMMAND x3d_sai_experimental_tests
    "--test-case=public semantic ranges stay pure derived and authored-order stable")
add_test(NAME sai_order_public_ranges COMMAND x3d_sai_experimental_tests
    "--test-case=public semantic ranges stay pure derived and authored-order stable")
add_test(NAME sai_field_specialized_access COMMAND x3d_sai_experimental_tests
    "--test-case=specialized containment mutation obeys the field lifecycle")
add_test(NAME sai_route_access COMMAND x3d_sai_experimental_tests
    "--test-case=routes connect output-capable ports to input-capable ports")
add_test(NAME sai_route_unique COMMAND x3d_sai_experimental_tests
    "--test-case=duplicate route identity is rejected without publication")
add_test(NAME sai_inspect_descriptor COMMAND x3d_sai_experimental_tests
    "--test-case=inspection discovers node types and ordered field definitions")
add_test(NAME sai_decl_identity_order COMMAND x3d_sai_experimental_tests
    "--test-case=declarations have stable identity and authored order")
add_test(NAME sai_decl_interface_validation COMMAND x3d_sai_experimental_tests
    "--test-case=declaration interfaces validate exact field semantics")
add_test(NAME sai_decl_interface_metadata COMMAND x3d_sai_experimental_tests
    "--test-case=prototype interfaces cannot redeclare inherited metadata")
add_test(NAME sai_decl_node_default_authority COMMAND x3d_sai_experimental_tests
    "--test-case=*node defaults*")
add_test(NAME sai_decl_node_defaults COMMAND x3d_sai_experimental_tests
    "--test-case=local PROTO node default*")
add_test(NAME sai_decl_failure_nonmutation COMMAND x3d_sai_experimental_tests
    "--test-case=rejected declaration edits publish nothing")
add_test(NAME sai_decl_mutation_duals COMMAND x3d_sai_experimental_tests
    "--test-case=declaration query and mutation operations are identity duals")
add_test(NAME sai_decl_snapshot_history COMMAND x3d_sai_experimental_tests
    "--test-case=declaration query and mutation operations are identity duals")
add_test(NAME sai_decl_update_intent COMMAND x3d_sai_experimental_tests
    "--test-case=declaration updates preserve kind and separate rename intent")
add_test(NAME sai_decl_change_order COMMAND x3d_sai_experimental_tests
    "--test-case=graph and declaration changes share one authored order")
add_test(NAME sai_decl_template_scope COMMAND x3d_sai_experimental_tests
    "--test-case=local declarations exclusively claim their template closure")
add_test(NAME sai_decl_scope_claim_atomic COMMAND x3d_sai_experimental_tests
    "--test-case=template claims reject live references and cycles atomically")
add_test(NAME sai_decl_scope_live_reference COMMAND x3d_sai_experimental_tests
    "--test-case=template claims revalidate live references at commit")
add_test(NAME sai_decl_live_observer COMMAND x3d_sai_experimental_tests
    "--test-case=template claims reject observers and undrained node payloads")
add_test(NAME sai_decl_scope_leakage COMMAND x3d_sai_experimental_tests
    "--test-case=template scope rejects public aliases routes and removal")
add_test(NAME sai_decl_scope_release COMMAND x3d_sai_experimental_tests
    "--test-case=removing a declaration releases rather than deletes template nodes")
add_test(NAME sai_decl_event_scope COMMAND x3d_sai_experimental_tests
    "--test-case=template nodes cannot participate in live event delivery")
add_test(NAME sai_decl_external_state COMMAND x3d_sai_experimental_tests
    "--test-case=external declarations expose honest explicit load states")
add_test(NAME sai_decl_external_authority COMMAND x3d_sai_experimental_tests
    "--test-case=*external resolution*,raw foreign declaration ID cannot resolve an external")
add_test(NAME sai_decl_external_compatibility COMMAND x3d_sai_experimental_tests
    "--test-case=resolved externals require and retain compatible local identity")
add_test(NAME sai_decl_external_subset COMMAND x3d_sai_experimental_tests
    "--test-case=resolved EXTERNPROTO interfaces are named subsets without defaults")
add_test(NAME sai_own_node_value COMMAND x3d_sai_experimental_tests
    "--test-case=node-valued writes require a context-bearing node handle")
add_test(NAME sai_own_node_range COMMAND x3d_sai_experimental_tests
    "--test-case=node ranges validate every element before staging")
add_test(NAME sai_cap_predictive COMMAND x3d_sai_experimental_tests
    "--test-case=experimental SAI starts as an empty descriptor-governed context")
add_test(NAME sai_edit_atomic_indexes COMMAND x3d_sai_experimental_tests
    "--test-case=names and routes publish at the same revision as their nodes")
add_test(NAME sai_edit_failure_nonmutation COMMAND x3d_sai_experimental_tests
    "--test-case=one invalid write poisons the edit and publishes nothing")
add_test(NAME sai_edit_revision_changeset COMMAND x3d_sai_experimental_tests
    "--test-case=change sets preserve authored operation order")
add_test(NAME sai_edit_query_mutation_dual COMMAND x3d_sai_experimental_tests
    "--test-case=enumerated roots names and routes are their own removal tokens")
add_test(NAME sai_conc_boundary COMMAND x3d_sai_experimental_tests
    "--test-case=optimistic edits conflict while old snapshots remain immutable")
add_test(NAME sai_life_stale_handle COMMAND x3d_sai_experimental_tests
    "--test-case=world replacement invalidates handles without invalidating snapshots")
add_test(NAME sai_life_node_removal COMMAND x3d_sai_experimental_tests
    "--test-case=unreferenced node removal preserves historical snapshots")
add_test(NAME sai_life_remove_references COMMAND x3d_sai_experimental_tests
    "--test-case=node removal rejects retained semantic references")
add_test(NAME sai_life_remove_nonmutation COMMAND x3d_sai_experimental_tests
    "--test-case=node removal rejects retained semantic references")
add_test(NAME sai_life_remove_atomic COMMAND x3d_sai_experimental_tests
    "--test-case=explicit detachment and node removal publish atomically")
add_test(NAME sai_life_wrapper_disposal COMMAND x3d_sai_experimental_tests
    "--test-case=wrapper disposal is local idempotent and callback free")
add_test(NAME sai_life_removed_handle COMMAND x3d_sai_experimental_tests
    "--test-case=removed live handles become stale and node IDs are not recycled")
add_test(NAME sai_life_removed_event COMMAND x3d_sai_experimental_tests
    "--test-case=node removal makes an older event batch stale")
add_test(NAME sai_life_observer_reference COMMAND x3d_sai_experimental_tests
    "--test-case=field observation retains its node until cancellation")
add_test(NAME sai_life_queued_event_reference COMMAND x3d_sai_experimental_tests
    "--test-case=undrained event notification retains its target node")
add_test(NAME sai_evt_observer_mutation COMMAND x3d_sai_experimental_tests
    "--test-case=commit queues observation and drain permits a reentrant commit")
add_test(NAME sai_evt_callback_copy COMMAND x3d_sai_experimental_tests
    "--test-case=dispatch does not copy user callables")
add_test(NAME sai_cxx_safe_destruction COMMAND x3d_sai_experimental_tests
    "--test-case=subscription cancellation and destruction are callback-free")
add_test(NAME sai_cxx_typed_generic COMMAND x3d_sai_experimental_tests
    "--test-case=dynamic discovery and typed fields are substitutable")
add_test(NAME sai_cxx_failure_value COMMAND x3d_sai_experimental_tests
    "--test-case=one invalid write poisons the edit and publishes nothing")
add_test(NAME sai_cxx_result_composition COMMAND x3d_sai_experimental_tests
    "--test-case=SAI results compose through expected-style monadic operations")
add_test(NAME sai_generated_typed_access COMMAND x3d_sai_experimental_tests
    "--test-case=generated field keys remove dynamic authoring lookup")
add_test(NAME sai_generated_binding_parity COMMAND x3d_sai_experimental_tests
    "--test-case=generated typed keys and metadata are one schema view")
add_test(NAME sai_generated_registry_provenance COMMAND x3d_sai_experimental_tests
    "--test-case=generated typed creation rejects mismatched registry provenance")
add_test(NAME sai_generated_registry_mutation COMMAND x3d_sai_experimental_tests
    "--test-case=mutating a generated registry revokes generated provenance")
add_test(NAME sai_generated_binding_drift COMMAND x3d_sai_experimental_tests
    "--test-case=generated key catalog has exhaustive ordered metadata parity")
add_test(NAME sai_generated_error_parity COMMAND x3d_sai_experimental_tests
    "--test-case=generated typed and dynamic failures have one vocabulary")
add_test(NAME sai_evt_stale_async COMMAND x3d_sai_experimental_tests
    "--test-case=cancelled and stale load completions are inert")
add_test(NAME sai_evt_input_transient COMMAND x3d_sai_experimental_tests
    "--test-case=inputOnly event intent is transient and access governed")
add_test(NAME sai_evt_inputoutput_publish COMMAND x3d_sai_experimental_tests
    "--test-case=inputOutput event intent publishes one coherent retained revision")
add_test(NAME sai_evt_typed_generic COMMAND x3d_sai_experimental_tests
    "--test-case=typed and dynamic event intent have equal traces and failures")
add_test(NAME sai_own_event_nodes COMMAND x3d_sai_experimental_tests
    "--test-case=node event payloads require context-bearing handles and ranges")
add_test(NAME sai_evt_route_causal COMMAND x3d_sai_experimental_tests
    "--test-case=a multi-hop route cascade preserves time and causal identity")
add_test(NAME sai_evt_route_loop COMMAND x3d_sai_experimental_tests
    "--test-case=a route cycle reaches quiescence by firing each route once")
add_test(NAME sai_evt_fanin_portability COMMAND x3d_sai_experimental_tests
    "--test-case=fan-in preserves the trace and exposes nonportable ordering")
add_test(NAME sai_evt_duplicate_seed COMMAND x3d_sai_experimental_tests
    "--test-case=duplicate event seeds poison the complete batch")
add_test(NAME sai_evt_field_observer COMMAND x3d_sai_experimental_tests
    "--test-case=field event observation is queued cancellable and access checked")
add_test(NAME sai_evt_reentrant_batch COMMAND x3d_sai_experimental_tests
    "--test-case=field observers isolate exceptions and permit a later reentrant batch")
add_test(NAME sai_evt_time_monotonic COMMAND x3d_sai_experimental_tests
    "--test-case=event publication rejects timestamps that do not move forward")
add_test(NAME sai_context_export_isolation COMMAND x3d_sai_experimental_tests
    "--test-case=context exports are ordered aliases while ordinary lookup stays local")
add_test(NAME sai_context_import_authority COMMAND x3d_sai_experimental_tests
    "--test-case=imports cross only explicit same-browser apertures")
add_test(NAME sai_context_alias_collision COMMAND x3d_sai_experimental_tests
    "--test-case=DEF and import names share one atomic local collision domain")
add_test(NAME sai_context_aperture_revalidate COMMAND x3d_sai_experimental_tests
    "--test-case=source export remapping invalidates a staged import atomically")
add_test(NAME sai_context_import_removal COMMAND x3d_sai_experimental_tests
    "--test-case=ordered imports are their own removal tokens")
add_test(NAME sai_life_import_reference COMMAND x3d_sai_experimental_tests
    "--test-case=committed imports retain source nodes across contexts")
add_test(NAME sai_life_import_race COMMAND x3d_sai_experimental_tests
    "--test-case=a committed import wins over an already staged source removal")
add_test(NAME sai_context_import_snapshot COMMAND x3d_sai_experimental_tests
    "--test-case=imported typed and dynamic reads capture immutable source revisions")
add_test(NAME sai_context_import_firewall COMMAND x3d_sai_experimental_tests
    "--test-case=imported authority is inspection-only and context checked")

add_executable(x3d_sai_experimental_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/author_inspect.cpp")
target_link_libraries(x3d_sai_experimental_example PRIVATE
    x3d_sai_experimental)
target_compile_features(x3d_sai_experimental_example PRIVATE cxx_std_20)
add_test(NAME x3d_sai_experimental_author_inspect
    COMMAND x3d_sai_experimental_example)

add_executable(x3d_sai_experimental_generated_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/generated_author_inspect.cpp")
target_link_libraries(x3d_sai_experimental_generated_example PRIVATE
    x3d_sai_experimental_metadata)
target_compile_features(x3d_sai_experimental_generated_example PRIVATE
    cxx_std_20)
add_test(NAME sai_generated_author_inspect
    COMMAND x3d_sai_experimental_generated_example)

add_test(NAME sai_generated_semantic_fields COMMAND x3d_sai_experimental_tests
    "--test-case=generated node interfaces distinguish fields from document syntax")
add_test(NAME sai_generated_typed_workflow COMMAND x3d_sai_experimental_tests
    "--test-case=typed discovery*")
add_test(NAME sai_node_null_access COMMAND x3d_sai_experimental_tests
    "--test-case=nullable node*,null SFNode*")
add_test(NAME sai_node_null_events COMMAND x3d_sai_experimental_tests
    "--test-case=null node*,null SFNode*")

add_test(NAME sai_field_snapshot_view COMMAND x3d_sai_experimental_tests
    "--test-case=snapshot field views*,imported field views*")

add_test(NAME sai_life_setup_across_revisions COMMAND x3d_sai_experimental_tests
    "--test-case=node setup*")
add_test(NAME sai_life_realization_access COMMAND x3d_sai_experimental_tests
    "--test-case=node setup*,attaching a generated*")
add_test(NAME sai_life_realization_authority COMMAND x3d_sai_experimental_tests
    "--test-case=realization rejects*")
add_test(NAME sai_life_realization_mf COMMAND x3d_sai_experimental_tests
    "--test-case=captured initializeOnly*")
add_test(NAME sai_life_realization_events COMMAND x3d_sai_experimental_tests
    "--test-case=the first event*")
add_test(NAME sai_life_realization_cycles COMMAND x3d_sai_experimental_tests
    "--test-case=realization preserves*")

add_executable(x3d_sai_runtime_tests tests/runtime_boundary_test.cpp)
target_link_libraries(x3d_sai_runtime_tests PRIVATE
    x3d_sai_experimental_metadata x3d_sai_doctest_main Threads::Threads)
add_test(NAME sai_runtime_cascade COMMAND x3d_sai_runtime_tests
    "--test-case=runtime outputs settle with causal routes in one revision")
add_test(NAME sai_runtime_atomic COMMAND x3d_sai_runtime_tests
    "--test-case=failed runtime evaluation publishes no partial frame")
add_test(NAME sai_runtime_authority COMMAND x3d_sai_runtime_tests
    "--test-case=runtime output authority expires and validates every payload,escaped runtime input facts survive destruction of their host")
add_test(NAME sai_runtime_reentrant COMMAND x3d_sai_runtime_tests
    "--test-case=runtime callbacks can inspect but cannot publish reentrantly,world replacement retains the old mutex and retires the processor outside locks")
add_test(NAME sai_runtime_ignored COMMAND x3d_sai_runtime_tests
    "--test-case=ignored native inputs do not retain echo or notify")
add_test(NAME sai_runtime_tick COMMAND x3d_sai_runtime_tests
    "--test-case=autonomous runtime output advances time and breaks feedback")

add_executable(x3d_sai_generated_runtime examples/generated_runtime.cpp)
target_link_libraries(x3d_sai_generated_runtime PRIVATE x3d_sai_experimental_metadata)
add_test(NAME sai_runtime_generated_host COMMAND x3d_sai_generated_runtime)
set_tests_properties(sai_runtime_cascade sai_runtime_atomic sai_runtime_authority
    sai_runtime_reentrant sai_runtime_ignored sai_runtime_tick
    sai_runtime_generated_host PROPERTIES TIMEOUT 15)

add_test(NAME sai_runtime_units COMMAND x3d_sai_runtime_tests
    "--test-case=runtime reads and outputs convert authored units exactly once")
set_tests_properties(sai_runtime_units PROPERTIES TIMEOUT 15)

add_executable(x3d_sai_stateful_tests tests/stateful_runtime_test.cpp)
target_link_libraries(x3d_sai_stateful_tests PRIVATE x3d_sai_experimental x3d_sai_doctest_main)
add_test(NAME sai_runtime_state_publish COMMAND x3d_sai_stateful_tests
    "--test-case=stateful runtime checkpoints publish with fields and retain history")
add_test(NAME sai_runtime_state_abort COMMAND x3d_sai_stateful_tests
    "--test-case=stateful runtime failures abort the checkpoint and the settled cascade")
add_test(NAME sai_runtime_state_only COMMAND x3d_sai_stateful_tests
    "--test-case=stateful-only frames revise and stateless processing clears the checkpoint")
set_tests_properties(sai_runtime_state_publish sai_runtime_state_abort
    sai_runtime_state_only PROPERTIES TIMEOUT 15)

add_executable(x3d_sai_generated_stateful_runtime examples/generated_stateful_runtime.cpp)
target_link_libraries(x3d_sai_generated_stateful_runtime PRIVATE x3d_sai_experimental_metadata)
add_test(NAME sai_runtime_state_generated_host COMMAND x3d_sai_generated_stateful_runtime)
set_tests_properties(sai_runtime_state_generated_host PROPERTIES TIMEOUT 15)

add_test(NAME sai_runtime_state_failure_protocol COMMAND x3d_sai_stateful_tests
    "--test-case=runtime factory and finalizer failures preserve state and diagnostics")
add_test(NAME sai_runtime_state_retirement COMMAND x3d_sai_stateful_tests
    "--test-case=retired native checkpoints release resources outside browser locks")
set_tests_properties(sai_runtime_state_failure_protocol sai_runtime_state_retirement
    PROPERTIES TIMEOUT 15)

add_executable(x3d_sai_callback_lifecycle_tests tests/callback_lifecycle_test.cpp)
target_link_libraries(x3d_sai_callback_lifecycle_tests PRIVATE
    x3d_sai_experimental x3d_sai_doctest_main Threads::Threads)
add_test(NAME sai_callback_lifecycle COMMAND x3d_sai_callback_lifecycle_tests)
set_tests_properties(sai_callback_lifecycle PROPERTIES TIMEOUT 15)

add_test(NAME sai_runtime_state_queued_history COMMAND x3d_sai_stateful_tests
    "--test-case=queued runtime checkpoint facts survive later frames and nested drains")
set_tests_properties(sai_runtime_state_queued_history PROPERTIES TIMEOUT 15)

add_executable(x3d_sai_publication_tests tests/publication_integrity_test.cpp)
target_link_libraries(x3d_sai_publication_tests PRIVATE
    x3d_sai_experimental x3d_sai_doctest_main)
add_test(NAME sai_publication_integrity COMMAND x3d_sai_publication_tests)
set_tests_properties(sai_publication_integrity PROPERTIES TIMEOUT 15)

add_executable(x3d_sai_callback_ownership_tests tests/callback_ownership_test.cpp)
target_link_libraries(x3d_sai_callback_ownership_tests PRIVATE
    x3d_sai_experimental x3d_sai_doctest_main Threads::Threads)
add_test(NAME sai_callback_ownership COMMAND x3d_sai_callback_ownership_tests)
set_tests_properties(sai_callback_ownership PROPERTIES TIMEOUT 15)

add_test(NAME sai_val_no_secret_repair COMMAND x3d_sai_experimental_tests
    "--test-case=authoring validation*")
add_test(NAME sai_val_stable_diagnostic COMMAND x3d_sai_experimental_tests
    "--test-case=authoring validation reports stable cycle endpoints")
set_tests_properties(sai_val_no_secret_repair sai_val_stable_diagnostic
    PROPERTIES TIMEOUT 15)

add_test(NAME sai_life_import_retirement COMMAND x3d_sai_callback_lifecycle_tests
    "--test-case=import retention releases temporary owners after unlocking")
set_tests_properties(sai_life_import_retirement PROPERTIES TIMEOUT 15)

add_test(NAME sai_runtime_read_retirement COMMAND x3d_sai_callback_lifecycle_tests
    "--test-case=runtime reads release imported host resources without step locks")
set_tests_properties(sai_runtime_read_retirement PROPERTIES TIMEOUT 15)

add_test(NAME sai_runtime_emit_retirement COMMAND x3d_sai_callback_lifecycle_tests
    "--test-case=runtime output validation releases foreign hosts without step locks")
set_tests_properties(sai_runtime_emit_retirement PROPERTIES TIMEOUT 15)

add_test(NAME sai_package_consumer COMMAND "${CMAKE_COMMAND}"
    "-DSAI_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DSAI_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "-DSAI_CONFIG=$<CONFIG>"
    "-DSAI_GENERATOR=${CMAKE_GENERATOR}"
    "-DSAI_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DSAI_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
    "-DSAI_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/package_consumer.cmake")
set_tests_properties(sai_package_consumer PROPERTIES TIMEOUT 300)

add_test(NAME sai_field_unit_validation COMMAND x3d_sai_experimental_tests
    "--test-case=unit representation preserves validation priority*")
set_tests_properties(sai_field_unit_validation PROPERTIES TIMEOUT 15)

add_test(NAME sai_scene_metadata_values COMMAND x3d_sai_experimental_tests
    "--test-case=scene metadata preserves values and ordered discovery")
add_test(NAME sai_scene_metadata_publication COMMAND x3d_sai_experimental_tests
    "--test-case=scene metadata shares graph publication and queued observation")
add_test(NAME sai_scene_metadata_rejection COMMAND x3d_sai_experimental_tests
    "--test-case=scene metadata rejects conflicting poisoned and retired edits")
add_test(NAME sai_scene_metadata_history COMMAND x3d_sai_experimental_tests
    "--test-case=scene metadata survives runtime frames and world replacement")
set_tests_properties(sai_scene_metadata_values sai_scene_metadata_publication
    sai_scene_metadata_rejection sai_scene_metadata_history PROPERTIES TIMEOUT 15)
