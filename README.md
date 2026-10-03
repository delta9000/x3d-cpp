# Experimental C++ SAI semantic kernel

## About this repository

Split out of [x3d-cpp](https://github.com/delta9000/x3d-cpp) on 2026-07-19, where it
lived on the unmerged `sai/semantic-kernel` branch. The pre-split branch head is
preserved there as the tag `archive/sai-semantic-kernel`.

This repository has no build-time, link-time, or Python dependency on x3d-cpp. Its
generator reads a vendored, SHA-pinned copy of the Web3D Consortium's X3D Unified
Object Model directly, so both projects are independent consumers of the same
upstream specification.

The intended endgame is that this kernel becomes x3d-cpp's authoring API. The trigger
for that decision is the strict convergence gate, `mise run sai-conformance`, going
green; it is expected to fail until the SAI is complete. Work here is a future part of
x3d-cpp rather than a permanent fork.

Commit history before the seed commits of 2026-07-19 is provenance, not a buildable
sequence. Those commits predate this repository's build system, so `git bisect` does
not work back across the import boundary.

## Using the C++ SDK

The SDK requires CMake 3.24 and a C++20 compiler. Build and install a standalone
copy with:

```sh
cmake -S . -B build-sdk -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdk --parallel 3
cmake --install build-sdk --prefix /path/to/sdk
```

An application locates that prefix with `-DCMAKE_PREFIX_PATH=/path/to/sdk` and
links the target for the vocabulary it uses:

```cmake
find_package(x3d_sai CONFIG REQUIRED)
add_executable(world_editor main.cpp)
target_link_libraries(world_editor PRIVATE x3d::sai_metadata)
```

`x3d::sai` supplies the semantic kernel and
`<x3d/sai/experimental/kernel.hpp>`. `x3d::sai_metadata` adds the generated UOM
catalog, `<x3d/sai/experimental/metadata.hpp>`, and typed bindings such as
`<x3d/sai/experimental/bindings/Transform.hpp>`. Both targets propagate the
C++20 requirement, include directories, and platform thread support. The
installed SDK includes the pinned `tl::expected` header and its license;
Python and the generator are not needed to build an application.
Both SDK archives use position-independent code, so shared libraries and
renderer plugins can link the same targets.

Embedding the checkout uses the same targets:

```cmake
add_subdirectory(path/to/x3d-sai)
target_link_libraries(world_editor PRIVATE x3d::sai_metadata)
```

Tests and example executables are added only by standalone builds with
`BUILD_TESTING=ON`. The installed prefix can be relocated; its CMake files
contain no checkout paths. The C++ API remains experimental, and an explicit
package version request must match exactly. Packaging follows CMake's
[exported-target model](https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html).

## The proposal

This repository is a compiling proposal for the irreducible semantics of a
modern C++ X3D SAI. It is deliberately firewalled in
`x3d::sai::experimental`: the library uses the C++ standard runtime and platform
thread support, and does not modify or depend on x3d-cpp's runtime. Its C++20
`result<T>` is a public alias to the pinned, single-header `tl::expected` 1.3.1
(CC0-1.0), providing `and_then`, `transform`, `transform_error`, and `or_else`
without a configure-time dependency or a homegrown expected implementation.

An optional `x3d_sai_experimental_metadata` bridge copies node and field facts
from an instance-free catalog generated from the UOM. The core kernel requires
only the C++ standard runtime and platform thread support; generated runtime
node objects never participate in SAI identity. The adapter currently fails closed for field types not yet present in
the experimental owning value vocabulary. The current generated vocabulary is
complete: all scalar and multi-field kinds have exact tags, owning storage, and
distinct strong types where C++ primitives would otherwise collapse X3D
semantics (for example, time and double). Bounded string fields retain their
normative `SFString` or `MFString` kind and use `std::string` or `string_list`.
Generated keys, dynamic discovery, indexed edits, and ROUTEs share those types.
`FontStyle.style` and `ScreenFontStyle.style` are semantic fields; the HTML
`style` attribute declared on `X3DNode` is excluded from the SAI catalog.
See the [X3D font and layout fields](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/layout.html#ScreenFontStyle).

The experimental `enum_value`, `enum_list`, `sf_enum`, and `mf_enum` extensions
have been removed. Replace their values with strings and string lists, and
inspect `sf_string` or `mf_string`. Vocabulary validation remains incomplete:
the UOM choice lists omit some normative values and mix individual flags with
whole sequences. For example, a single `"CENTER"` is a valid `Layout.align`
value, and font styles permit an empty string. The SDK preserves these values.
The [published physics vocabulary](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollisionCollection)
uses underscore spellings such as `SLIP_1`; the pinned UOM uses hyphens for
some flags. String values are retained exactly as supplied.

Catalog provenance carries the X3D specification version, a deterministic
SHA-256 fingerprint of the resolved semantic model, and the generator version.

The proposal makes these design choices executable:

- `node_id` is semantic identity; an `occurrence` is an ordered containment
  edge with its own parent, field, index, and path.
- An `execution_context` owns one generation. `scene_snapshot` is immutable;
  `scene_edit` stages an atomic revision and returns an ordered `change_set`.
  The context reserves node and declaration IDs when they are created, so an
  abandoned or conflicting edit can leave gaps but cannot lend its IDs to a
  later object. Local and external declarations share one ID sequence.
  For ordinary state changes, `before` and `after` are transition facts. The
  `unit_declared` kind is explicitly discriminated metadata: `field` is the
  category, `before` is the declared name, `after` is the canonical conversion
  factor, and `index` is declaration order; consumers must switch on `kind`.
- One `field_descriptor` governs dynamic and typed field handles. Access type
  is enforced as a lifecycle rule, and ordinary failure is a `result<T>` with
  structured `sai_error` data and expected-style monadic composition.
- `value_kind` has one public spelling: the lowercase form of the exact X3D
  field type (`sf_vec3f`, `mf_node`, `sf_color`, `sf_color_rgba`). C++ owning
  representations such as `vec3f`, `color3f`, and `color4f` describe storage;
  they do not introduce alternate schema-tag spellings.
- MF fields are edited through revision-bound typed or dynamic range editors,
  never borrowed mutable containers. Indexed changes remain ordered inside one
  atomic `scene_edit`; MFNode mutations accept only context-bearing handles and
  validate the complete replacement range before staging it.
- `readable()` predicts snapshot inspection. Intent-specific `writable()`
  distinguishes setup authoring, retained writes, event input, and host-only
  runtime output. Each operation uses the dynamic descriptor's access rules.
  Unit declarations are ordered scene facts: authored-space values cross one
  explicit conversion boundary while storage, ROUTEs, and changes stay in
  canonical space. Custom field and declaration-interface descriptors accept
  the X3D derived categories acceleration, angular_velocity, area, speed, and
  volume; their conversion uses the declared angle or length base factor.
  UNIT declarations remain limited to angle, force, length, and mass. A finite
  authored value whose converted result exceeds the field's finite range is
  rejected before a retained edit or event batch is published; authored reads
  report the same error when inverse conversion exceeds that range.
- Retained writes, event inputs, and native outputs validate destination
  ownership, lifecycle/access, applicable template restrictions, and payload
  type before converting units. Structural error codes, identities, and messages
  therefore agree between canonical and authored representations, including
  typed calls. An admitted value that overflows during conversion still returns
  `invalid_value` and publishes nothing.
- SFColor, SFColorRGBA, MFColor, and MFColorRGBA channels must lie in [0, 1],
  as specified by the X3D [color field types](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/fieldsDef.html).
  Authoring, event input, native output, and MF editors reject out-of-range,
  NaN, or infinite channels with `invalid_value`; rejection poisons the whole
  transaction without publishing earlier staged changes. Defaults in node
  descriptors and local declaration interfaces are checked on admission.
  Valid endpoints and signed zero retain their representation. MF editors also
  report malformed images as `invalid_value`, while a wrong field kind remains
  `type_mismatch`.
- Each field descriptor owns a `numeric_range`: optional minimum and maximum,
  with inclusive/exclusive flags. Generated UOM bounds survive catalog and node
  discovery and participate in the schema fingerprint. Custom descriptors use
  the same range. Authoring, event input, native output, MF edits, and ROUTE
  destinations check every numeric component before staging its value. Bounds
  use canonical units and are checked after authored-unit conversion. Floating
  endpoints use the component precision; integer bounds retain fractional
  limits. A bounded field rejects non-finite components, while an unbounded
  field retains its existing IEEE representation behavior.
  Descriptor admission rejects non-finite limits and component intervals with
  no representable values. Rejected writes return `invalid_value` and leave
  the whole revision unpublished.
  Input-only and output-only ports may initially hold their exact field-type
  default even when it lies outside the field's range; declared defaults and
  subsequent events must satisfy the range. For example, `aspectRatio` starts
  at zero but can emit only positive values. Numeric ranges do not implement
  coupled rules such as the all-axes `bboxSize` sentinel, or enum vocabularies.
- Field storage and event equivalence is the public `same_representation`
  relation: IEEE payload bits survive, signed zeros differ, and identical NaN
  payloads match. Ordinary numerical `operator==` remains numerical.
- Roots, ordered scoped names, and unique routes are inspected and edited
  through the same transaction. IDs found through inspection resolve back to
  handles, and mutation tokens require no private keys.
- Node removal is an explicit, non-cascading staged mutation. Roots, node-valued
  fields, names, exports, routes, and committed imports retain their targets
  until the caller removes or rewrites those references. Field observers and
  undrained event notifications retain targets at the live context boundary;
  old snapshots remain historical truth and current handles become stale after
  publication.
- `node::dispose()` is local wrapper invalidation: it is `noexcept`, idempotent,
  callback-free, identity-preserving, and has no scene or copy-wide effect.
- Node type and ordered field descriptors are discoverable as owning values.
  Node-valued writes require context-bearing `node` handles (or spans of them),
  preventing a bare ID from aliasing an object in another ownership domain.
  Local PROTO interface SFNode and MFNode defaults are authored through
  `scene_edit::set_declaration_default(proto, field, node_or_null)` and its
  `std::span<const std::optional<node>>` overload. For example, create a local
  declaration and node in one edit, then call
  `edit.set_declaration_default(proto.value(), "child", child.value())` before
  committing. The complete referenced node graph becomes declaration-owned;
  defaults and body roots share that scope. Replacing a default leaves its
  previously claimed nodes owned until the declaration is removed. Bare IDs
  remain inspection data and cannot author non-null defaults.
  PROTO and EXTERNPROTO interfaces cannot redeclare the inherited `metadata`
  field.
  EXTERNPROTO interfaces have no authored defaults. Their by-name kind/access
  fields must be compatible with the resolved local declaration; omitted
  optional annotations are unconstrained. A resolved target remains retained
  while the exposed interface fields are compatible. Resolve with
  `edit.resolve_external_declaration(external, local)` using declaration handles;
  `update_declaration` may edit metadata or reset load state, but cannot assign
  a new resolved target from a bare ID.
- Execution-context boundaries have explicit ordered EXPORT/IMPORT apertures.
  Imported nodes and fields are distinct inspection-only proxy types; parent
  snapshots capture immutable source revisions without claiming a globally
  atomic multi-context snapshot.
- Observation is explicit, queued, and drained outside internal locks.
  Recursive or concurrent drains return `already_draining` without consuming
  notifications; the active consumer preserves notification order. Callbacks
  run on the active drain caller's thread; choosing that thread sets affinity.
  A producer that publishes after the drain's final empty check needs a later drain.
  Cancellation suppresses callbacks not yet acquired by the drain, while an
  already acquired callback may finish. Callback captures are retired outside
  context and browser locks, including registration failure and world replacement.
  `subscription` is move-only RAII; cancellation and destruction invoke no
  callbacks.
- External event intent is a timestamped, move-only batch. `inputOnly` delivery
  is transient, `inputOutput` delivery publishes retained state once, and ROUTE
  propagation records causal predecessors while breaking loops per route.
  PROTO template nodes cannot receive live events or appear in their node-valued
  payloads; template changes remain authoring edits. Template claims cannot take
  nodes retained by field observers, undrained events, or committed imports.
- Field observation is access-governed and shares the explicit drain boundary.
  Fan-in remains inspectable as a deterministic trace plus a structured
  portability issue instead of silently blessing queue order as semantics.
- World replacement and asynchronous completion are generation/request aware,
  so stale handles and stale completions cannot mutate a replacement world.
- Capabilities fail closed: the kernel advertises authoring and inspection, but
  not live execution, loading, or rendering merely because their publication
  safety protocols can be exercised.

`examples/generated_author_inspect.cpp` is the primary authoring story. It
selects generated descriptors by tag, uses owner-specific generated keys and
typed nodes for field and graph writes, composes fallible work with `result<T>`,
commits once, and inspects the same typed fields. Generated tags and registries
carry the same model fingerprint; typed creation fails before producing a
handle when provenance differs. A successful public mutation of a generated
registry revokes that provenance, so mixed handwritten/generated descriptors
cannot acquire generated typed handles. The exhaustive generated-key catalog is checked
against metadata for every node and ordered field.
`examples/author_inspect.cpp` remains the deliberately handwritten dynamic
fixture for event, ROUTE, shared-node, and import behavior.
The focused doctest suite is also registered as stable `sai_*` CTests so each
demonstrated invariant can be cited independently by the convergence register.

## Discovering the configured vocabulary

`registry.node_types()` and `snapshot.node_types()` return owning
`node_type_descriptor` values sorted by name. The list contains the configured
built-in types, including abstract interfaces. Every descriptor includes its
ordered fields, exact value kinds, defaults, access, accepted node types, units,
component facts, and interface ancestry. Editing or retaining this returned
collection cannot change the registry or scene.

A palette or inspector can discover choices before creating any node:

```cpp
const auto choices = scene.snapshot().node_types();
for (const auto &type : choices) {
  if (type.abstract)
    continue;
  // Display type.name and choose field editors from type.fields.
}
```

After a concrete entry is selected, pass its name to `edit.create_node(type.name)`
and check the result before staging fields or roots.

The vocabulary stays with its execution-context generation and is inspectable
through historical snapshots after world replacement. It describes which node
and field facts the authoring kernel accepts. Runtime behavior, component-level
conformance, and complete profile support require their own capability evidence;
`capabilities().live` remains false. Local declarations have their existing
separate declaration discovery API.

## Typed and dynamic access

Generated bindings describe the abstract X3D node fields. Document constructs
(`DEF`, `USE`, `IS`) and HTML attributes (`class`, `id`, `style`) do not become
ordinary field/event ports. Use scoped names, shared handles, and declaration
operations for their semantic roles. Document syntax preservation remains part
of the future codec work. Independent node-interface tests supplement parity
between generated artifacts.

A tool can discover a node dynamically and specialize it through
`snapshot.as<bindings::Transform>(node)`. This checks context, node type, and
model provenance. Generated abstract interfaces also work: a Transform can be
viewed as `typed_node<bindings::X3DGroupingNode>`. Historical snapshots retain
that inspection ability after a world is replaced; publishing an edit against
a retired context fails.

Typed fields support `readable`, intent-specific `writable`, `add_route`, and
`observe`, as well as the existing reads, writes, and range edits. These paths
reuse the dynamic descriptor checks. `field.dynamic()` exposes the same handle
for algorithms that discover their field types at runtime; it grants no extra
access.

`observe`, `set_runtime_processor`, and `set_runtime_factory` accept move-only
callables directly, including lambdas that capture a `unique_ptr` or promise.
Pass a named move-only callable with `std::move(callback)` to transfer ownership.
The SDK retains the captured resources with the registration or installed host;
cancellation, host removal, and world replacement release them outside internal
locks. Copyable callables and the existing `std::function` aliases remain valid.

A default-constructed `node{}` is X3D NULL and carries no context authority.
`snapshot.read_node(field)` and `snapshot.read_nodes(field)` return usable
handles, preserving every NULL position in MFNode. The existing `read` operation
continues to return IDs as inspection data. Author with node handles, including
NULL, through `set`, events, or the MFNode range editor. All non-null values
retain type, context, and scope checks; passing raw IDs remains rejected.
`is_null()` tests absence, while `disposed()` reports local wrapper disposal.
Disposed wrappers cannot be used to author values.

`snapshot.read_view(field)` returns `result<std::shared_ptr<const T>>` for a
typed field and `result<std::shared_ptr<const value>>` for a dynamic field.
The standard shared pointer pins immutable canonical storage without copying
its payload. It stays valid after the snapshot, context, or browser is gone;
later revisions never change its contents. Imported views pin the exact source
revision and enforce the same aperture and access checks as owning reads.
Views carry inspection data, not mutation authority. Release them when finished:
a view retains its entire source revision. Use `read(field, value_space::authored)`
when a converted owning value is needed. Editing still copies the scene state.

`mise run gen-determinism` generates into a temporary directory and compares
both contents and file inventory. It never rewrites the current bindings and
works while source and generated changes are uncommitted. Set `TMPDIR` to select
the scratch location. `X3D_SAI_BUILD_DIR` selects the CMake/CTest directory
for both `mise run build` and `mise run ci`; its default is `build`. Builds use
three parallel jobs so the test and generated-catalog translation units do not
consume unbounded memory.

## Node setup and realization

A new node starts in setup. Committing an unattached node preserves that state,
so its initializeOnly fields can be configured in a later edit. The generated
consumer example authors WorldInfo.title across that boundary.
`edit.realize(node)` explicitly ends setup and is idempotent;
`snapshot.is_realized(node)` inspects the state at that revision. Both operations
accept generated typed nodes and dynamic handles.

Assigning a node-valued field or adding a child/root realizes the referenced
node graph in the same edit. The containing node remains in setup until it is
itself referenced, added as a root, or explicitly realized. NULL slots and shared
references produce no duplicate transitions. Removing a reference never reopens
setup. MF editors check access again at every mutation, including editors
obtained before their owner was realized. A first event realizes its target and
node payloads together with the event's retained effects.

Realization produces a `node_realized` graph change with an empty field name,
`before=false`, and `after=true`. Publication advances the revision and queues
observation through the existing commit boundary. Failed or conflicting edits
publish neither phase nor field changes; historical snapshots keep their phase.

This authoring boundary follows the setup/realized distinction in
[SAI 4.4.8](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html).
Snapshot reads are observational. Name/declaration metadata changes leave
realization unchanged.
Snapshots can inspect retained initializeOnly facts after realization. Live SAI
field access must enforce its lifecycle table at the live service boundary;
inspection does not stand in for that boundary. The semantic transition does
not yet initialize a renderer, Script, or runtime adapter.

## Scene metadata

Scene META statements are edited with the graph in the same atomic revision:

```cpp
auto edit = scene.edit();
if (!edit.set_metadata("title", "A world"))
  return;
if (!edit.set_metadata("obsolete", std::nullopt))
  return;
auto committed = edit.commit();
```

`snapshot.metadata("title")` returns an owning `std::optional<std::string>`;
absence differs from a present empty string. `snapshot.metadata()` exposes
immutable `metadata_entry` values in insertion order; keep the snapshot alive
while using that range. Replacement keeps a key's
position; removal followed by reinsertion appends it. Keys are case-sensitive.
Setting an identical value or removing an absent key adds no change.

Metadata changes are `metadata_change` alternatives in the ordered `change_set`,
with a key and optional `before`/`after` strings. Graph and metadata changes
publish together and use the same queued observation boundary. Metadata does
not emit field events or realize nodes. Historical snapshots retain their
metadata, including after runtime frames and world replacement.

The zero-or-one value and NULL-removal rules follow the
[SAI scene services](https://web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-2v4.0-CD/Part02/services.html#getMetaData).
These are document META facts; node metadata remains ordinary node/field data.
Parsing, serialization, and Script access remain open conformance work.

## Evaluating runtime events

The browser owner can install a `runtime_processor` for one of its scenes with
`browser::set_runtime_processor(scene, callback)`. Passing an empty callback
removes it. A callback receives a `runtime_step`; execution-context clients
cannot construct one or obtain output authority through a write-intent flag.

Each `event_batch::commit()` with a processor first calls it once with
`step.input() == nullptr`. That preparation call can emit sensor outputs from
`step.time()`, including for an otherwise empty batch. The kernel then invokes
the processor for each external or routed input. Native outputs go through
ROUTEs; they do not recursively invoke the processor as inputs. All deliveries
in the settled cascade share the batch timestamp. A successful processor frame
advances the time floor even if it produces no events. Later frames must have
strictly increasing finite timestamps.

`step.read(field)` returns an owning copy of the working value. It sees prior
processed deliveries and the current tentative inputOutput value; committed
scene snapshots still show the previous revision until publication. inputOnly
payloads are available through `step.input()->payload`. Both typed and dynamic
reads and outputs support explicit authored-unit conversion. `step.emit(field,
value)` queues outputOnly or inputOutput events; node-valued outputs take local
handles or spans, preserving NULL entries and applying the usual scope and
type checks. Queued outputs become working values when processed after the
callback. No mutable storage or snapshot view is exposed to the host.
Local `runtime_step::read()` and output-authority validation acquire no
foreign context owners. A foreign host released during either operation can
inspect local runtime fields from its destructor. Validation errors identify
`runtime_step.read` or `runtime_step.emit`, with the offending node identity
and field name when applicable.

A native rule can call `step.ignore_input()` to reject an incoming value without
failing the frame. Its node is realized, but the payload is not retained,
echoed, or observed. `read()` then sees the prior stored value. A host may emit
an adjusted inputOutput value; that explicit output replaces the automatic
echo of the incoming value. The ledger retains incoming events and marks
ignored ones. Field observations report emitted outputs. Per-field and
per-ROUTE limits allow one output per timestamp, including when the graph feeds
native output back into its input. Every attempted output is validated before
loop suppression, so a repeated field cannot hide a foreign or malformed value.

The returned `event_result` distinguishes `external`, `route`, and `runtime`
origins. Runtime outputs point to their input's ledger index through `cause`;
preparation outputs have no cause. ROUTE deliveries also carry `route_index`.
State changes and queued observations become visible only after the whole
cascade succeeds. A returned error, processor exception, or invalid output aborts field
storage, realization changes, queues, revisions, and time advancement. Copies
of a `runtime_step` keep its input and time facts alive, but reads and outputs
return `stale_revision` after the callback returns.

Processor callbacks run outside context locks. They can inspect committed
snapshots or manage subscriptions. Concurrent or reentrant publication to that
context, processor replacement, and world replacement return
`operation_in_progress`. A load completion rejected at this boundary retains
its ticket for retry. Retired callbacks are destroyed outside browser and context
locks. This synchronous boundary reports processed completion
when `commit()` returns; asynchronous acceptance and presentation scheduling
remain separate work.

[generated_runtime.cpp](examples/generated_runtime.cpp) is an independent host
using generated PositionInterpolator and Transform fields. It reads key data,
computes interpolation, emits `value_changed`, and checks routed translation and
settled observation, including endpoint clamping. It links only this SAI
project. This example proves the host boundary; it does not implement every
Interpolation component behavior. Its requirements follow the
[X3D event model](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Eventmodel)
and [PositionInterpolator](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#PositionInterpolator).

### Stateful hosts

A stateful host installs `browser::set_runtime_factory(scene, factory)`. The
factory receives the immutable base snapshot and frame time, and returns
`result<runtime_frame>`. Wrap an ordinary moveable object whose public members
are `result<void> process(runtime_step&)` and `result<State> finish()`. The
`runtime_frame_adapter` concept checks this protocol. Neither the frame nor
its state needs to be copyable; host code does not inherit an SDK base class.

The factory runs once before preparation. The kernel processes the cascade,
then calls `finish()` once. It moves the returned state into an owned const
checkpoint and publishes it with field values and realization in one revision.
A factory, processor, or finalizer error aborts that frame, including routed
changes, observation queues, and time advancement. Frame resources are released
through their ordinary destructors outside context locks on both success and
failure. Factories and finalizers have the same reentrant publication limits
as processor callbacks.

`scene_snapshot::runtime_state<State>()` returns
`result<std::shared_ptr<const State>>`: successful null means there is no
checkpoint, while requesting a different state type returns `type_mismatch`.
The shared pointer pins the checkpoint; historical snapshots keep their previous
state. A `runtime_state_change` alternative in `semantic_change` owns the
before/after checkpoints and exposes the same typed queries. A stateful frame
advances the revision even when its X3D fields remain equal.

Both installation methods replace the same host evaluator. Passing an empty
factory or processor disables evaluation and leaves the last checkpoint intact.
Installing a stateless processor clears that checkpoint on its next successful
frame. Authoring edits preserve it without evaluating the host; derived host
facts are refreshed at the next runtime frame.

[generated_stateful_runtime.cpp](examples/generated_stateful_runtime.cpp) uses
only the public SDK and generated RigidBody and Transform bindings. Its small
free-motion evaluator integrates `position` from `linearVelocity`, emits
position through a ROUTE to translation, and owns a move-only checkpoint. It
checks late finalizer failure, retry at the same timestamp, coherent observation,
and retained history. Velocity inputs change the following interval; incoming
position events are ignored by this host. Other RigidBody fields, including
forces, torques, mass, damping, gravity, orientation, geometry, and collision
behavior, are not evaluated. This is an adapter example with limited motion
semantics, not RigidBodyPhysics conformance.

Hosts must stage private working resources and return an owned checkpoint.
Const state access does not prevent mutation through aliases kept by host code;
the adapter must preserve the checkpoint's immutability. The kernel cannot undo
external I/O or mutations made outside this protocol. The `live` capability
stays false: this boundary does not supply the remaining live lifecycle and
browser services.

## Publication guarantees

Authoring commits and event cascades validate node references and the declared
containment graph before publication. A containment cycle rejects the entire
commit; shared nodes, NULL slots, and non-containment reference cycles remain
valid. Node-valued events, including transient inputs and routed deliveries,
obey the same accepted-node-type and declaration-scope checks as authoring.
These hierarchy checks follow
[X3D 4.3.3 and 4.3.5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Scenegraphhierarchy);
the reference kernel rejects an invalid candidate instead of publishing it.

Commit prepares its result, notifications, candidate state, and import retention
before changing any published state. An allocation exception during publication
propagates as `std::bad_alloc` and leaves the visible revision, field values,
queued observations, event time, and import retention unchanged. Exceptions at
host callback boundaries follow the runtime failure rules above. Native frame
and checkpoint resources are destroyed outside context locks on failed
publication, so their cleanup can inspect the committed scene.
Admission checks that borrow an importer keep that owner until context locks
are released. Releasing its last external handle during a check therefore
leaves its host resource cleanup free to inspect the source scene, including
when allocating the rejection diagnostic fails.

`scene_edit::validate() const` preflights the same checks as `commit()`:
previous mutation errors, consumed edits, declaration compatibility, node
references and containment, current revision/lifecycle, live retention, and
import source exports. It returns the existing `result<void>` and `sai_error`
data without consuming the edit, repairing its graph, realizing nodes, or
publishing state or observations. It releases its context locks before returning.
Success reserves nothing; an intervening edit, retirement, observation, import,
or runtime evaluation can change whether the later commit is admitted.

Containment diagnostics identify the node and field whose edge closes a cycle.
Validation traverses nodes by identity and fields/children in their declared
order, making that endpoint deterministic. An earlier mutation error retains
its original operation and identity. Source locations, severity, and complete
causal diagnostic chains remain future work.

A failed validation leaves the staged edit intact. A commit-time graph error
can be corrected through explicit further edits and validated again; a mutation
that already poisoned the edit requires a fresh edit. Discard a failed event
batch and start a fresh one. Publication atomicity does not cover arbitrary
mutations of a thread-confined staged edit or external host side effects.

## Deliberate limits

This is a semantic reference kernel, not a complete ISO/IEC 19775-2 or
19777-4 implementation. It currently omits parsing and serialization,
profiles and document-codec unit integration, PROTO/EXTERNPROTO expansion,
cross-context field writes and ROUTEs, Inline loading, PROTO/IS
apertures, Script integration, URL
transport, concrete browser adapters, presentation scheduling, scene-change
provenance, and an ABI-stable boundary. Generated catalog provenance is
explicit. EXPORT/IMPORT currently provides snapshot
inspection,
not multi-context live publication. The event kernel is a synchronous reference
model, not a claim that the complete live SAI capability is available.
The load API models only cancellation and stale-completion publication rules.

Live lifecycle remains partial: browser-driven initialization, Script phases,
live read restrictions, and observation-triggered realization need a host
boundary. The kernel's historical inspection surface deliberately stays pure.
Atomic `scene_edit` is an authoring extension; it does not yet implement SAI
session update buffering or accepted/published/processed completion for a live
host. The synchronous event model cannot establish those guarantees.

Handwritten type registries remain explicit test fixtures for fast iteration.
Production-facing experiments can instead select descriptors from the
generated UOM catalog; unsupported value kinds are reported rather than
silently narrowed if a future model extends the vocabulary. The current 4.0
UOM does not formally annotate per-field
unit categories, so the catalog preserves that absence and the unit-overlay
design remains open. Dynamic typed fields remain checked views over the
descriptor. Generated owner-specific keys provide the direct authoring lane
without changing capabilities or bypassing descriptor validation. Typed reads
defensively report a structured type mismatch rather than exposing variant
access exceptions.

No API in this directory should be promoted merely because it compiles. The
invariant tests and composed examples are the review surface: revise the
experimental vocabulary until those user stories remain clear and the
remaining planned invariants can be expressed without privileged access.

## Conformance register review

Run `mise run sai-conformance-gate` and `mise run sai-invariants` in this
repository to check the service register, invariant links, CTest names, and
generated `docs/conformance/SAI-BASELINE.md`. These checks also run in
`mise run ci`. After editing either YAML register, run `mise run sai-baseline`
and commit the generated baseline with the register change. Run
`mise run sai-conformance` separately to see the remaining full SAI gaps; it
is expected to fail while services are partial or planned.
The normal checks validate register consistency and drift. They do not require
every ISO service or C++ binding obligation to be mapped; the baseline lists
those gaps, and the strict gate requires complete coverage.

The register includes partial native services and explicitly planned document
ingestion and ABI policy. Responsibility links and service counts do not
establish ISO conformance. Ordinary allocation failures may throw
`std::bad_alloc`; semantic failures use `result<T>`.

For SFNode fields, `edit.set(field, std::nullopt)` and
`events.send(field, std::nullopt)` assign NULL, as do nullable `node{}` handles.
MFNode handle ranges preserve positional NULL entries. Bare IDs remain rejected.
