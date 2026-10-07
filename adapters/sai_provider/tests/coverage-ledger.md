# Native provider assertion migration ledger

Baseline: offline common oracle 1,199 checks per backend, hosted common oracle
160 checks per backend, native-only authoring proofs and six native runtime
cases. Consolidation changes composition and handles, not the retained evidence.
Common fixture accounting is maintained in the independent x3d-sai consolidated
provider fixture and its migration ledger; native checks below remain separate.

| Former source and proof | Consolidated source | Disposition |
|---|---|---|
| tests/cmake/sai_provider/native.cpp native_authority_proof | same file | All authoritative native node/root/DEF/readback, wrong-thread, retained-storage and expired-authority assertions preserved through test-local final-service convenience fixture |
| native_scalar_authority_proof | same file | All seven scalar kinds, defaults, exact-kind rejection, invalid rotations, unchanged failure effects, repaired reads and authored presence preserved |
| Old initializeOnly bboxSize unavailable-write assertion | native_scalar_authority_proof | Intentional capability expansion: replace unavailable setup write with positive exact setup write/presence assertion and negative live write assertion |
| native_user_data_authority_proof | same file | Native values/graph remain untouched by node/field metadata, canonical alias slots, destructor timing, retained payload and handle teardown preserved |
| native_children_proof | same file | NULL/order/uniqueness, cross-parent aliases, detached foreign pointer/duplicate/cycle rejection, full-state unchanged failures and local repair preserved |
| native_deep_detached_graph_proof | same file | Full 4,096-node detached chain/cycle/repair preserved |
| adapters/sai_hosted/tests/hosted_native_test.cpp native_route_and_render | adapters/sai_provider/tests/native_runtime_test.cpp | Real interpolation/ROUTE/native storage, tick/time, deferred event order, owning copies, transform-only RenderDelta, equal-time turns and empty-tick evidence preserved |
| all_four_payload_kinds | native_runtime_test.cpp | Setup authorship, coupled live MF configuration and float/vector/array conversion evidence preserved |
| independent_scene_retirement | native_runtime_test.cpp | Admission budgets, terminal receipts, independent clocks/storage, retirement isolation and extractor continuity preserved |
| partial_failure_no_replay | native_runtime_test.cpp | Post-mutation failure effects, native readback, failed/cancelled receipts, output discard, no replay and fresh-world evidence preserved |
| activation_failure_retirement | native_runtime_test.cpp | Partially built native session faults, no retry, explicit retirement and subsequent activation preserved |
| destruction_during_deferred_notification | native_runtime_test.cpp | Owner deletion from deferred callback, suppression of remaining calls, expired authority and terminal receipts preserved |
| Unrelated native author-field sentinel | native_runtime_test.cpp main | Uses independent Scene.authorFields owner; every native scene leaves unrelated author state untouched |
| Shared hosted three-root bootstrap | testing::make_provider_route_scene(service&) | Generic public setup creates same rooted transforms/interpolator and nested Shape/Box renderable geometry; no production fixture-only scene API is needed |
| Two source/relocated harnesses | tests/cmake/sai_provider + verify_sai_provider.sh | One integration harness runs both former suites, native evidence and parity in shared/static modes; exact native link remains reference-kernel/metadata-free |

New evidence beyond the baseline:

- native_sfnode_authority_proof verifies real Shape.geometry/Box storage,
  owner-bearing reads, cross-service rejection, NULL, foreign native-pointer
  repair, activation identity and retirement
- ordered_inputs_bounded_outputs distinguishes three identical/distinct input
  occurrences in one tick from one generated output/ROUTE and coherent readback
- unnamed_routes_and_weak_identity verifies generic detached interpolator and
  unnamed endpoint routing, retirement of retained native handlers,
  no fabricated DEFs, duplicate rejection, default name-bound refresh and
  expired direct endpoint non-resurrection
- shared_root_behavior_once preserves repeated root occurrences without duplicate
  behavior/output attachment
- all_four_payload_kinds now also verifies normative pre-input getter initialization
  from nonempty/empty setup keyValue, before and after activation

- native_geometry_constraints_proof verifies Box.size strictly-positive and
  bboxSize nonnegative-or-exact-sentinel domains, signed bboxCenter, native
  storage/authored-mark preservation on failure, malformed native read rejection
  and valid-write repair. Shared canonical validation owns these rules; the
  native backend does not link reference metadata or duplicate a shadow schema.

- native_scalar_bit_pattern_proof checks native and portable signed-zero,
  infinity and NaN payload bits for ordinary Transform vectors and rotation
  angles, preserving the original scalar domain rather than importing bounded
  interpolation restrictions into unrelated fields

The test-local native_authoring_fixture.hpp is not installed. It delegates all
operations to the one final service, using the same owner-bearing handles and
ordinary generated keys; it is not a compatibility production API.
