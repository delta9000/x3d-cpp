# One provider-owned SAI contract

## Status

The canonical application header is `provider.hpp`, in
`x3d::sai::experimental::provider`. The final `service` owns one `backend` by
composition. Both reference and native storage implement that interface; setup
and live operations do not switch handle families. `offline_provider` and the
separate hosted frontend have been removed rather than kept as additional
application APIs. This is source-level experimental interoperability for the
advertised subset, not a stable binary ABI or complete ISO conformance.

The scalar/MF vocabulary and generated field keys still come from the pinned
schema. Public SFNode/MFNode payloads hold `optional<node>` and owning vectors of
those references. Only `nullopt` is NULL. Empty, disposed, foreign or expired
engaged wrappers do not grant node authority. Raw numeric node IDs are an internal
backend representation, not an application authoring capability.

## Queued live ROUTE addition

`service_id::live_routes` is an optional version-1 capability. With it,
`enqueue_route(client, from, to)` returns an owning ticket using the same FIFO,
update gate, cancellation, budgets, and retirement rules as field writes. The
existing `add_route(context, from, to)` remains setup-only. Backends without the
capability reject admission with `unsupported_operation`; the new backend
virtual also defaults to unsupported without effects, so existing subclasses
remain source compatible. Rebuild frontend and backend together; the added
virtual has no stable binary ABI promise.

Admission validates the client and canonical fields, service/scene authority,
lexical context, exact portable kind, and ROUTE direction. Sources must be
outputOnly or inputOutput; destinations must be inputOnly or inputOutput.
Unsupported field services are refused. Admission then calls the optional
`backend::do_check_live_route(from, to)`, which defaults to admit. A backend
uses it for restrictions the portable descriptors cannot express. For example,
the native companion's SFEnum and SFString fields share portable `sf_string`,
but a ROUTE between them is refused at enqueue. Installation at pump may still
refuse, for example when the graph changed after admission. Those refusals
appear in the turn report and receipt rather than the enqueue result.

An exact duplicate ROUTE (same source node and field, same sink node and field)
is redundant and succeeds without effect (ISO 19775-1 4.4.8.2). When a gate is
released, the frontend keeps only the first copy of each route held in it, and
the duplicate's ticket shares the submission outcome. Deduplicating at release
rather than at enqueue keeps cancellation of a held copy exact.
`do_add_live_route` must return success without effect when the route is
already installed. Setup-time `add_route` still refuses duplicates.

A route request consumes one ordinary FIFO submission when no update gate is
open. It installs before that turn's backend evaluation. Earlier submissions
run without the route, and installation does not replay previous outputs. An
empty write vector still performs the route turn's ordinary backend tick.
Within a released update gate, all route requests install in their relative
order before evaluating all retained writes and input occurrences in that
submission, even if a write was enqueued before a route. Every receipt in that
gate shares its submission and outcome. Cancelling any one cancels the whole
submission, as with grouped field writes.

Admission counts each route call against `pending_requests` and accounts one
`backend::admitted_route` object plus both canonical field-name byte counts
against `payload_bytes`. It excludes allocator overhead and backend topology
storage. Held calls retain that accounting until processing or cancellation.

The backend implements `do_add_live_route(from, to)` and returns an explicit
failure effect. A refusal before effects leaves the activation active. If any
route in a submission was installed and a later installation or evaluation
fails, the frontend reports `partial_or_unknown`, faults the activation, and
cancels its later requests. No rollback of earlier installed routes is implied.
Application callbacks remain deferred through explicit notification dispatch.
`backend::observed(address)` tells a backend whether a live observer watches a
field, so it can skip reading and recording writes that nobody observes.

The reference provider uses its existing scene-edit transaction to install the
route. The native companion updates executable state and the retained native
document, while refusing nested contexts and PROTO interface redirection. Route
removal remains outside this capability. The own-suite `sai_provider_contract`
cases cover FIFO ordering, gates, cancellation, authority/kind/direction,
budgets, defaults, partial installation, redundant duplicates, and admission
pre-checks. `sai_provider_reference` checks actual next-turn delivery and
duplicate no-ops alone and in a gate.

## Ownership and operation matrix

`scene`, lexical `context`, `node` and canonical `field` handles carry provider
and scene identity. A scene's activation and its logical turn are separate
identities. Retiring and later reusing an internal numeric ID cannot resurrect
an old handle. Consumer-held identity tokens retain no backend scene storage.

| Operations | Allowed state / outcome | Classification |
| --- | --- | --- |
| create scene/node, name, root, author, add route | Setup only; exact kind/access/authority checks | Bounded portable setup contract |
| roots, named lookup/enumeration, reverse names, type/field discovery, readable field values | Setup, active or faulted; inputOnly read denied | SAI-derived service semantics |
| activate | Ends setup for the scene's owned nodes; failure can leave it faulted | Explicit binding lifecycle policy |
| enqueue_route | Active, optional `live_routes`; installs at its FIFO turn | Portable optional capability |
| connect, begin/end, enqueue | Active activation; per-client gate and owning requests | SAI-derived buffering plus explicit receipt policy |
| pump | Active only, finite nondecreasing host time; no hidden scheduler | Host extension |
| observe/drain | Deferred owning notifications; cancellation and callback affinity explicit | SAI-derived observation plus C++ scheduling policy |
| user data | Immediate node/canonical-field application metadata, independent of values/revisions | Owning C++ mapping |
| retire | Setup/active/faulted; removes live registries and backend resources | Explicit lifecycle policy |
| status of owning ticket | Thread-safe snapshot, including after service destruction | C++ receipt extension |
| close | Checked owner-thread operation; explicit failure before teardown if in flight | C++ owner contract |

Normal service operations are owner-thread-affine. Retained thread-lifetime
tokens prevent a recycled OS thread ID from acquiring authority. Copying owning
values/receipts across threads does not authorize concurrent scene mutation.
Subscription cancellation is thread-safe; already executing callbacks are not
interrupted. Checked `close()` returns an error on the wrong thread or during a
backend turn; destruction requires the owner thread unless already closed.

The frontend owns the backend and shutdown sequence. Backend event sinks retain
only weak frontend state. Teardown revokes authority before releasing application
captures. Application callbacks and capture destructors may reenter or destroy
the service; dispatch pins detached state and does not resume through a destroyed
owner. Native-only extraction/inspection remains a clearly separate extension.

## Named-node discovery

`named_nodes(context)` returns an owning vector of `named_node { name, target }`
entries in ascending, case-sensitive name order. It includes every name in that
lexical context, including named nodes detached from roots; unnamed nodes do not
appear. `named(context, entry.name)` resolves to `entry.target`. The current
provider contract has one root lexical context per scene; discovery does not
flatten future nested contexts or enumerate IMPORT aliases.

`names(node)` returns an owning vector of names bound to that node in its lexical
context, in the same order. An unnamed node succeeds with an empty vector. The
current setup contract still permits only one canonical DEF name per node;
this query does not introduce alias authoring or rename operations.

Both queries follow the discovery authority and owner-thread rules and work in
setup, active, and faulted states. Disposed, foreign, and retired handles fail
as they do for `named` and `type_name`. Returned strings and vectors survive
later edits, retirement, and service destruction; retained node handles do not
retain backend storage or authority after retirement.

Backends implement `do_named_nodes(scene)` using their existing name bindings.
The frontend sorts, rejects null identities, invalid names, and duplicate names
with `invalid_descriptor`, and wraps IDs in provider-owned handles. Reverse
lookup filters that same enumeration; it keeps no second name table. The default
virtual returns `unsupported_operation`, preserving source compatibility for
existing backend subclasses. As with other virtual additions, rebuild backend
and frontend together; this experimental interface has no stable binary ABI.

## Scene units and metadata

The optional `scene_units` and `scene_metadata` capabilities are implemented by
both bundled backends. Storage belongs to the backend, never a parallel
frontend scene model. Unadvertised capabilities and default backend methods fail
closed. All operations use the existing checked scene/context authority and
owner-thread rules; owning results survive later edits, retirement and service
destruction.

`units(context)` returns the four effective categories, in angle/force/length/mass
order. Undeclared categories report radian/newton/metre/kilogram and factor 1.
`declare_unit(scene, declaration)` accepts one explicit declaration per category,
with a nonempty name containing no ASCII whitespace and a finite positive factor.
It is setup-only and must precede the first successful node creation, including
detached nodes. Invalid calls leave existing declarations usable. Time is always
seconds; derived dimensions are not separately declared.

All provider field authoring, readback, event values and presentation remain in
canonical SI units. Declarations describe document units; they do not rescale
these API values. The native adapter marks provider-authored scalar fields as
already normalized, preventing a second conversion during activation. Generated
defaults and live writes remain canonical. This does not add authored-space field
input, parsing or serialization. A future serializer must inverse-convert relevant
canonical values or change declarations; emitting canonical bytes beside
nonidentity UNIT statements would change the scene on reparsing.

`metadata(scene)` returns owned key/value entries in first-insertion order;
`metadata(scene, key)` returns an optional owned string. Missing differs from a
present empty string. Keys are case-sensitive, and empty keys are preserved.
`set_metadata(scene, key, value)` replaces a value without changing its position;
`nullopt` removes it, and reinsertion appends. Scene META is distinct from node
metadata fields and application user data. Setup changes emit no field events
and do not open client gates, activate nodes or consume logical turns.

Both discoveries work in setup, active and faulted states. Metadata mutation is
immediate and setup-only. It does **not** implement the standard live buffered
setMetaData service. Importing duplicate document META names, optional XML META
attributes, nested execution contexts and lossless codec round trips remain open.
See [scene-information contract and standards mapping](scene-information.md).

## One gate and submission model

The selected abstract SAI defines per-session buffering and release order:
[4.8.4.3](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html#ExtUpdatingSceneGraph),
[4.8.4.5](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html#SynchronizingMultipleApplications)
and [6.3.19](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#UpdateControl).

- One non-nesting gate per client; repeated Begin and unmatched End are no-ops
- End releases one submission. Release time determines FIFO order across clients
  of the same activation. The host still chooses which independent activation to pump
- Per-call owning tickets retain correlation IDs; released calls share one
  mutex-protected outcome. A submission ID is not a sortable FIFO sequence
- Stateful whole-field assignments use a documented last-accepted-value policy
  within the gate. Intrinsic errors reject a call without poisoning earlier work
- Every explicit whole inputOnly assignment is an ordered occurrence, including
  repeated identical values. It is not silently treated as retained state
- MF indexed operations aggregate into the pending field candidate in call order;
  the last accepted write to an index wins. For inputOnly MF, indexed edits alter
  the most recent pending owning occurrence; absence of such a base is rejected
- Coupled constraints are checked on the final released candidate and immediately
  before execution. A held gate may repair intermediate coupled inconsistency

The mixed whole-inputOnly/indexed-MF rule is an explicit proposal decision. The
published [4.3.5.5.3 indexed aggregation rule](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/concepts.html#Set1ValueExternalInteractions)
does not fully settle mixed whole-value occurrences. It must not be presented
as an unambiguous inherited requirement. Browser/load/topology/dispose ordering
is not completed by this field-submission slice.

Accepted, submitted and processed are different states. Processed means the
released work was evaluated, including documented coalescing; it does not mean
every intermediate state value appeared as a separately visible event. Dropping
a receipt does not cancel accepted work. Terminal payloads leave service queues;
consumers choose how long to retain their receipt metadata.

## Time, failures and observations

Each admitted pump has a distinct logical turn and finite nondecreasing host
time, including equal times. Release-time rejection consumes no turn; execution
preflight attempts consume a turn even when they make no backend mutation. This
is a host integration policy, not a promise of reference revision parity.

A known-no-mutation preflight failure fails its submission and leaves the
activation healthy. A backend failure with partial or unknown effects faults
that activation and cancels remaining work. A successful outer `result` may
contain a faulted turn report; callers inspect the report. Native rollback and
automatic replay are never implied. Callback failure does not undo native work.

All stateful candidate inputs precede cascade evaluation; inputOnly occurrences
retain their accepted order. This does not promise a distinct output for each
input. The architectural [output/ROUTE limits](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#ExecutionModel)
and loop breakers remain in force for a cascade; no epsilon timestamps or
per-input guard resets are introduced. Tests compare required causality and
coherence, not an arbitrary fan-in winner.

Application notifications run only in a separate explicit drain. Already
completed notifications survive a later failed turn; failed-turn notices are
discarded. Cancellation or retirement revokes pending delivery. New notices
created by a callback wait for a later drain; recursive drains are rejected.

## Explicit resource admission

`resource_limits` is constructor configuration, with defaults:

- 1,024 pending requests and 16 MiB accounted owning request payloads, including
  held gates
- 1,024 live registrations
- 4,096 queued notifications and 16 MiB accounted stored notification payloads

Checked arithmetic detects overflow; zero-length payloads still consume their
write/field-name storage. Predictable full queues reject before backend entry
without losing earlier accepted work. Overflow discovered during execution has
honest partial/unknown effects and a faulted activation, not silent event loss.
Cancelled registrations are reclaimed on owner-thread cleanup. Retired scene
records and terminal request history are not kept in immortal indexes.

These are admission budgets, not total process-memory limits. Allocator overhead,
spare capacity, backend resources, application captures and consumer-retained
values/receipts are outside them. Stored shared payload accounting is explicit;
it does not claim the same physical bytes were allocated once per reference.
FIFO selection is constant-time in historical request count; group cleanup is
proportional to its per-call receipt count, not an unlimited-history scan.

## Capabilities and optional reference extensions

Backend service/node/field support must describe executable behavior. Schema
`required_component` and `minimum_declared_level` tags are membership facts,
not a declaration of complete component/profile support. Per-field supported
flags remain authoritative for the bounded adapter. Exact generated-key owner
matching is a current restriction; complete interface inheritance projection
is not claimed.

The current ordinary factory/workload covers advertised Transform,
PositionInterpolator, Shape and Box behavior, with owner-bearing node values,
ROUTEs and actual native execution/extraction. It does not provide general
PROTO/IMPORT/Script/geospatial/URL/codec/browser service conformance. Native
runtime ownership fixes do not automatically expose those services through SAI.

`kernel.hpp`, `scene_snapshot`, `scene_edit`, strict-time `external_session`,
runtime checkpoints and construction-only initialization remain explicitly
reference-only mechanisms. They do not define another portable handle family.
Their atomicity/history policies are not imposed on the native backend.

Use `x3d::sai` for the common frontend, `x3d::sai_reference_provider` for its
reference backend, or the separately installed native companion. The reference
kernel/metadata are explicitly named `x3d::sai_reference_kernel` and
`x3d::sai_reference_metadata`. Only public include roots are exported.

See the [coverage ledger](provider-fixture-coverage-ledger.md) for retained and
changed tests. Exact-head CI, compiler/platform breadth, fully instrumented
runtime testing and a real CAVEOS integration remain separately attributed
milestones; no CAVEOS source was available for this work.
