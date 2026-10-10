# Historical hosted experiment

Superseded by [the consolidated provider contract](portable-contract.md).
This records the earlier prototype and its limits; its target/header names are
not current application interfaces.

# Experimental hosted runtime slice

`x3d::sai_hosted` is an additive, backend-neutral source contract for an explicitly
host-driven runtime. It does not replace `offline_provider`, kernel handles,
`events(time)`, or `external_session`. It does not claim full ISO SAI, profile,
component-level, browser, or CAVEOS support.

The first proof is deliberately narrow but executes real behavior:

```
portable enqueue(set_fraction)
  -> native PositionInterpolator input handler
  -> native value_changed storage/event
  -> authored native ROUTE
  -> native Transform.translation
  -> native world transform and RenderDelta
```

The native implementation is the optional `x3d-cpp/adapters/sai_hosted` companion.
The reference implementation is `x3d::sai_reference_hosted`. Only the latter
links the semantic kernel and generated SAI metadata. The native adapter links
real x3d-cpp domain-runtime libraries and the portable hosted contract, with no
window, renderer backend, or reference evaluator dependency.

## What is implemented

- Owner-thread-affine service ownership and independently owned scenes
- Separate opaque scene, lexical context, node, and canonical field identities
- Roots, DEF lookup, type and field discovery, and inputOutput aliases
- The existing owning SAI value vocabulary and generated field keys, with
  executable SFFloat, MFFloat, SFVec3f, and MFVec3f support in this slice
- Checked setup authoring, initializeOnly setup access, and compatible ROUTEs
- Activation, isolated client update gates, accepted/submitted/processed tickets
- Explicit, finite, nondecreasing host time and a distinct logical turn serial
- Queued runtime input, deferred cancellable output notifications, and retirement
- Explicit failed/cancelled statuses and native mutation effect reporting

`create_route_fixture()` is explicitly a backend-owned construction seam, not a
portable arbitrary scene importer/factory. It constructs PositionInterpolator,
Transform, and Mirror roots, initialized to a two-point curve, with an authored
Interpolator-to-Transform route. Native-only Shape/Box children supply extraction
evidence. The common frontend adds the second, checked route to Mirror.

Unsupported fields remain discoverable with `supported == false`; reads return
`unsupported_field_type`. The first slice excludes general scene construction,
node-valued payloads, live topology edits, PROTO, IMPORT/EXPORT, Script,
geospatial behavior, URLs, codecs, resource loading, and asynchronous completion.
It introduces no file or network I/O. The native DynamicFieldStore now belongs
to each Scene/standalone execution context, and RuntimeSession uses its document's
owner. Dynamic author-field admission remains outside this hosted fixture. The
GeoFrame projection service still needs explicit ownership before geospatial
features can join the contract. Built-in reflection may consult the scene-owned
author-field fallback; this fixture never installs or mutates geospatial state.

## A host turn

```cpp
#include <x3d/sai/experimental/reference_hosted.hpp>
namespace sai = x3d::sai::experimental;
auto service = sai::make_reference_hosted().value();
auto scene = service->create_route_fixture().value();
auto context = service->root_context(scene).value();
auto interpolator = service->named(context, "Interpolator").value();
auto fraction = service->get_field(interpolator, "set_fraction").value();
auto activation = service->activate(scene).value();
auto client = service->connect(activation).value();
auto ticket = service->enqueue(client, {{fraction, 0.5f}}).value();
// Nothing has evaluated yet.
auto turn = service->pump(activation, sai::event_time{10.0}).value();
service->dispatch_notifications().value();
service->retire(activation).value();
```

Production callers must inspect each `result`; the compact example uses
`value()` only to show sequencing. `pump` processes at most one released request
batch for that activation, in release FIFO order. An empty pump still advances
native behavior. A held gate belongs to its client: another client can submit
and process work while it is held. `end_update` releases that client's accepted
requests; it does not tick the world. Each enqueue remains a separate scheduled
batch, even when released by the same gate. Repeated `begin_update` and unmatched
`end_update` are errors. These experimental hosted gate rules deliberately remain
different from `external_session`'s idempotent, coalescing gates; no equivalence is
claimed. A future shared gate policy requires a separate compatibility decision.
A ticket is an owning receipt: `ticket.status()` returns a synchronized snapshot
of its current or terminal state, and `ticket.id()` supplies its correlation ID.

The host clock may repeat. Two calls at `10.0` are distinct turns; decreasing or
nonfinite times are rejected before any turn starts. No clock, thread, event
loop, renderer, or application callback is started secretly.

The bounded interpolator contract requires nonempty strictly increasing keys
and the same number of keyValue elements. Adjacent key differences and keyValue
component differences must be representable as finite binary32 values. Duplicate
keys and overflow-prone interpolation spans are explicitly rejected in this
slice rather than hiding native/reference differences. This is a scoped
admission restriction, not a change to existing native interpolation semantics or
a claim that all ISO-valid curves are supported. Portable numerical results are
subject to backend floating-point arithmetic; the exact cross-backend fixture
comparison uses exactly representable observations. Setup writes can temporarily violate that coupling,
so callers can resize key then keyValue. Activation and complete batches must be
valid. Validation uses current observed state plus the complete batch, and runs
again immediately before execution because earlier queued batches may have
changed it. Within a batch, key/keyValue inputs are seeded before other inputs,
stably within each group. A set_fraction listed first therefore still evaluates
the complete admitted curve. This is an explicit hosted submission rule; native
`postEvent` and existing external-session ordering are unchanged. There is no
general total-order or ROUTE fan-in winner guarantee.

## Authority and lifetime

A handle's identity consists of backend authority, stable scene identity, lexical
context identity, node identity, and (for a field) canonical field name. The
current fixture has one root lexical context. It does not pretend that a native
pointer is a general PROTO identity: logical outer/nested instances can share a
physical endpoint and require a future explicit scope model.

Activation epoch and turn are separate from object identity. Acquiring a field
through `translation`, `set_translation`, or `translation_changed` yields the
same canonical identity. Wrapper `dispose()` affects only that wrapper. Copies
and later reacquisitions remain usable while their owning scene is live.

Retirement cancels unprocessed requests, invalidates scene operations, suppresses
queued notifications, and destroys the backend scene/runtime. It leaves other
scenes operable. Owned receipts retain terminal status across disconnect,
retirement, close, and service destruction. Copies share only receipt metadata;
no receipt owns backend storage, a scene, service authority, or request payloads.
An empty or moved-from receipt returns `stale_handle` from `status()`. `retire(scene)` also handles setup scenes and activation
construction failures. There is no implicit scene reactivation/reload or auto
retry; prepare a new independently owned scene. Closing expires service authority
without retaining native pointers in public handles.

Receipt `status()` snapshots are thread-safe, including while the owner thread
processes or cancels the request. Receipt copies/destruction follow ordinary C++
shared ownership rules: a caller must not mutate the same wrapper concurrently.
Dropping every caller-owned receipt does not cancel accepted work. Cancellation
is explicit: `cancel(client, ticket)` checks service and client authority and is
owner-thread-affine. A numeric correlation ID alone grants no authority.

Service operations are owner-thread-affine. Authority uses a retained thread
identity token, so OS thread-ID reuse cannot accidentally authorize another
thread. A wrong-thread service call returns `wrong_thread`. Subscription token
cancellation/destruction is safe across threads: it atomically suppresses future
callbacks and does not interrupt a callback already acquired for execution.
Owning callback captures are released later on the service thread.

Application notification callbacks may close or destroy the owning service;
dispatch pins detached state and never accesses the destroyed service afterward.
Move-only callbacks are accepted through owning C++20 wrappers. Capture destructors
may also reenter service cleanup. Derived implementations call `close()` before
destroying backend storage. Closing or destroying during an active backend turn
is prohibited and fails closed rather than tearing down a native stack.

## Bounded request ownership and migration

The service keeps only live requests, in activation-local release FIFOs and
client-held queues. Completion, cancellation, disconnection, fault, retirement,
and close physically remove terminal entries and release their owning payloads.
There is no completed-request map, weak diagnostic history, or historical pump
scan. Admission/release/processing maintain FIFO directly; idle selection is
constant-time and independent of earlier completed work. Terminal receipt memory
is retained exactly by consumers who still own a receipt, outside the admission
budget. Receipt destruction never calls back into a service.

`request_limits` defaults to 1024 pending requests and 16 MiB of accounted owning
payload bytes per service, shared by every scene and client, including gated
accepted work. Set both at construction with `make_reference_hosted(limits)` or
native `SaiHostedService(limits)`. Zero means no admission for that budget. Full
capacity returns `error_code::resource_limit` before admission or backend mutation;
earlier accepted requests remain unchanged and capacity recovers on removal.

Every write costs `sizeof(admitted_write)` (inline address, string and payload
objects), the canonical field-name byte length, and dynamic logical payload
storage. Variable-length values add element count times element size; bool lists
add one byte per element; strings and images add their owned data bytes. Lists of
strings/images add their element objects plus each nested allocation's logical
byte length. Empty variable-length values still pay for a write and its field
name. Multiplication and addition are checked against a remaining-byte budget
before arithmetic, so overflow cannot wrap into an accepted request. This is an
admission accounting policy, not a total allocator or process-memory bound:
allocator metadata, spare capacity, backend storage, notifications and
consumer-retained receipts are outside it.

This experimental ownership correction intentionally changes source usage:

- `ticket.id` becomes `ticket.id()`
- `ticket.state` and `service.status(client, ticket.id)` become `ticket.status()`
  followed by inspection of the returned `request_status`
- `service.cancel(client, ticket.id)` becomes `service.cancel(client, ticket)`
- `make_reference_hosted()` still works; passing its function as a no-argument
  callable now needs `[] { return make_reference_hosted(); }`, because the factory
  accepts an optional `request_limits` argument

An ID cannot recover a dropped receipt. Consumers needing terminal results retain
those receipts explicitly. This avoids a hidden service-lifetime history cost.
Disconnected and retired clients are removed. Minimal retired scene records remain
until service destruction to preserve `state(scene) == retired`; they do not own
native scene storage or request payloads. Cancelled observer captures and queued
notices are intentionally released at the next explicit notification drain or
close, preserving owner-thread capture cleanup and reentrancy. These existing
scene/notification lifetimes are separate from the request admission budget.

## Failure and notification semantics

Admission errors return without queuing or mutating runtime state. Activation
configuration preflight errors preserve setup for correction. Once backend
activation construction has begun, failure faults the scene instead of claiming
pristine setup; it can be explicitly retired.

An execution-time failure faults the activation and cancels its remaining queued
requests. Output captured during the failed turn is discarded, but undrained
notifications from earlier successfully completed turns remain deliverable even
while the activation is faulted. Cancellation, retirement, or close can still
revoke those pending deliveries. `failure_effect::none` states that no backend field mutation occurred
in the failed execution (for example, revalidation failed before native entry).
`partial_or_unknown` means native effects may already have happened. A successful
`pump` result can carry a faulted `turn_report`; callers must inspect `state` and
`error`, not merely the outer `result`. No native rollback, immutable snapshot,
revision parity, or safe replay is promised. After a native exception the adapter
never retries that turn automatically.

Native listeners copy owning output payloads during the cascade. Application
callbacks run only in `dispatch_notifications()`, after the host turn returns.
Cancellation or retirement suppresses already queued notices. Source output
causally precedes its routed destinations; sibling fan-out order is not a portable
requirement. Callback exceptions are reported by the drain after other callbacks
run, do not roll back completed native work, and are not replayed. A callback may
submit or pump more work; notices produced by it wait for a subsequent explicit
drain. Recursive notification drain is rejected.

## Reference clock extension

The reference backend uses the new explicit
`execution_context::logical_turn(event_time)` extension. Its finite clock is
nondecreasing and each successful commit opens a separate evaluation/cycle scope,
even at equal time. Empty logical turns also participate in its admission clock.
It shares the context clock with `events(time)`, whose existing strictly increasing
policy is unchanged. `external_session` continues to use that strict policy.
Failed logical commits do not advance the kernel admission clock. No kernel
revision is exposed as the portable hosted turn identity.

## Falsification evidence

`testing/hosted_fixture.hpp` is the same frontend source compiled for both real
stores. It compares read outcomes, identity/isolation, access errors, ticket
states, and normalized causal notifications, never kernel revisions or fan-in
winners. It exercises initialization, same-time turns, client gates, cancellation,
malformed values/routes, paired curve resizing, stale queued configuration,
callback exceptions, retirement, and wrong-thread entry.

The native-only executable additionally inspects copies derived from native
PositionInterpolator/Transform storage, worldTransform, and RenderDelta. It
injects a failure after a real native tick to prove partial effects, faulted
activation, cancelled remaining work, and no automatic replay. A separate parity
executable links both adapters only for tests; the native-only executable and
adapter remain reference-kernel-free.
