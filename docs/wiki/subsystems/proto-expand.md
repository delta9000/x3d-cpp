---
title: "PROTO/EXTERNPROTO Expansion"
summary: Clones and splices PROTO/EXTERNPROTO instances into the live scene graph after parsing, forwarding field values and wiring IS-connections.
tags: [subsystem, proto, externproto, expansion, is-connection]
updated: 2026-09-29
related:
  - ../architecture.md
  - ../subsystems/parse-readers.md
  - ../subsystems/scene-graph.md
  - ../subsystems/routes.md
  - ../subsystems/execution-context.md
---

# PROTO/EXTERNPROTO Expansion

## Purpose

PROTO/EXTERNPROTO expansion is the post-parse pass that turns structural prototype records into live scene-graph nodes. After any reader (XML, VRML, JSON, Classic VRML) has built the raw document, the scene holds `ProtoInstance` records but the node graph has no expanded content from them. The expansion subsystem iterates those records, deep-clones the prototype body for each instance, forwards `initializeOnly`/`inputOutput` field values through IS-connections, pre-resolves body-internal ROUTEs to concrete cloned endpoints, registers event-redirect maps for exposed interface fields, and splices the primary cloned node back into its parent slot (or the scene root). For body-nested `ProtoInstance`s, reader-captured `nodeField IS protoField` mappings on the nested instance are applied before recursive expansion, so outer overrides/defaults propagate across proto boundaries (including `MFNode` chains). EXTERNPROTO instances resolve through a configurable `ProtoDeclarationResolver` callback; local PROTOs expand without any I/O. The pass is lenient: every failure becomes a `ProtoWarning` collected in `X3DDocument::protoWarnings`, never an exception that aborts the parse.

The boundary owned by this subsystem is: from collected `Scene::protoInstances` (structural, reader-produced) to expanded primary nodes in the scene graph with `Scene::resolvedProtoRoutes`, `Scene::protoRedirects`, and `Scene::expandedSources` populated.

## Key files

| File | Role |
|---|---|
| `runtime/X3DProtoExpand.hpp` | Entry points `expandScene` and `expandInstance`; `ExpandGuard` depth limiter; `proto_detail` helpers (`findField`, `interfaceField`, `instanceValue`, `attachToParent`) |
| `runtime/X3DProtoClone.hpp` | `deepClone` — deep-copies a node tree via the reflection layer; preserves DEF/USE shared identity within the clone; `FallbackNodeCreator` hook for ext nodes |
| `runtime/X3DProto.hpp` | Data model: `ProtoDeclaration`, `ExternProtoDeclaration`, `ProtoBody`, `ProtoField`, `ProtoFieldValue`, `ProtoInstance`, `IsConnection`, `ResolvedProtoRoute`, `ProtoRedirect`, `ProtoWarning` |
| `runtime/parse/X3DProtoResolver.hpp` | `ProtoDeclarationResolver` function type + `noopProtoResolver` default |
| `runtime/parse/AssetProtoResolver.hpp` | `protoResolverFrom(AssetResolver)` — EXTERNPROTO resolution over the AssetResolver seam |
| `runtime/parse/X3DParse.hpp` | Front door `parseDocument` — invokes `expandScene` after the reader and range-warning passes; defines `localFileProtoResolver` (file-local default) |
| `runtime/X3DScene.hpp` | `Scene` — owns `protoInstances`, `resolvedProtoRoutes`, `protoRedirects`, `expandedSources`, `protoDeclarations`, `externProtoDeclarations` |

## Interfaces and seams

### Exposed interface

Successful EXTERN resolution retains the selected `ProtoDeclaration` on the
instance and its `Scene::expandedSources` record. Its `sourceUnits` snapshot
preserves the defining document's UNIT declarations. Each explicit expansion
attempt still calls the supplied resolver; a failed resolution clears the
instance's retained declaration. The retained source factors and
authored-field marks are consumed when the expanded graph enters runtime.
Conversion applies only to authored concrete fields with a known dimension;
a custom PROTO interface value uses its source-unit provenance and the
dimension of each concrete IS target, so one interface value can be converted
differently at separate targets. See `runtime/UnitConversion.hpp` and
[Execution Context](execution-context.md).

`ProtoDeclaration::authoredScalarFields` retains parse-time field presence.
Expansion copies those marks onto cloned body nodes, interface node defaults,
and literal node values on body-nested instances. All declaration-owned node
graphs, including interface defaults and body nodes in one declaration DEF
scope, use one clone map per instance, preserving DEF/USE aliases within that
instance while keeping separate instances and the declaration independent.
Caller-supplied node overrides retain their shared identity. A present empty
node override forwards `NULL`/`[]` rather than falling back to the interface
default. Expansion also records successful scalar IS assignments, including
explicit values equal to generated defaults.
The marks describe parsed initialization; later application writes do not
update them. Runtime normalization uses these marks to distinguish explicit
values from built-in defaults, and records normalized fields to prevent a
second application of the conversion factor.

```cpp
namespace x3d::runtime {

// Front door: expand every ProtoInstance captured in `scene`.
// `resolver` resolves EXTERNPROTO url lists to their declaration.
// `baseUrl` is the directory of the source file (for relative url resolution).
// Diagnostics land in `warnings`; exceptions never escape.
void expandScene(Scene &scene,
                 const x3d::codec::ProtoDeclarationResolver &resolver,
                 const std::string &baseUrl,
                 std::vector<ProtoWarning> &warnings);

// Low-level: expand a single instance. Returns null + a ProtoWarning on any
// failure (unresolved EXTERN, missing/empty body, recursion depth exceeded).
std::shared_ptr<X3DNode>
expandInstance(ProtoInstance &inst, Scene &scene,
               const x3d::codec::ProtoDeclarationResolver &resolver,
               const std::string &baseUrl,
               ExpandGuard &guard,
               std::vector<ProtoWarning> &warnings);

// Recursion/cycle guard. maxDepth = 32.
struct ExpandGuard { int depth = 0; int maxDepth = 32; };

// Deep-clone a node tree. cloneMap preserves DEF/USE shared identity.
std::shared_ptr<X3DNode>
deepClone(const std::shared_ptr<X3DNode> &src,
          std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> &cloneMap);

} // namespace x3d::runtime

namespace x3d::codec {

// Resolver callback type. Must not throw; return null for any unresolvable url.
using ProtoDeclarationResolver =
    std::function<std::shared_ptr<x3d::runtime::ProtoDeclaration>(
        const std::vector<std::string> &urls, const std::string &baseUrl)>;

// No-op resolver: always returns null (local PROTOs still expand).
std::shared_ptr<x3d::runtime::ProtoDeclaration>
noopProtoResolver(const std::vector<std::string> &, const std::string &);

} // namespace x3d::codec
```

Both `expandScene` and `expandInstance` are inline, header-only functions in `runtime/X3DProtoExpand.hpp`. `deepClone` is inline in `runtime/X3DProtoClone.hpp`.

### Seam points

- **`ProtoDeclarationResolver` callback** — the single pluggable seam for EXTERNPROTO resolution. The default (`localFileProtoResolver` in `runtime/parse/X3DParse.hpp`) resolves file-relative urls, parses the target file, and returns its matching `ProtoDeclaration`. It skips http/https/urn urls (embedder-override territory) and uses a `thread_local` active-file stack to terminate cross-file EXTERN cycles. Embedders replace this with a network fetch, virtual FS, or content-addressable cache. The ext firewall wires its own resolver via `x3d::runtime::ext::install()` (see `runtime/ext/ExtResolver.hpp`), which intercepts ExternalGeometry URNs before falling through to the base resolver.

- **`protoResolverFrom` adapter** — `runtime/parse/AssetProtoResolver.hpp` adapts the renderer-agnostic [`AssetResolver`](system-asset-io.md) seam into a `ProtoDeclarationResolver`, so an embedder can plug a routed http/s3 backend into parse-time expansion. `protoResolverFrom(AssetResolver, Encoding hint = Unknown)` walks each url in order, strips a `#ProtoName` fragment, fetches the document bytes (`AssetKind::ExternProto`), sniffs the encoding, parses with `parseDocument`, and returns the named declaration (fragment) or the document's first. `Failed` skips to the next candidate; `Pending` is a hard error at this parse-time call site (contract B) and yields null; it never throws. `urn:` resolves only when the injected resolver owns that scheme (e.g. a `makeSchemeRouter` with a `"urn"` entry). Like the default resolver, it keeps a `thread_local` set of urls currently being resolved, so a self- or mutually-referencing EXTERNPROTO terminates (a re-entered url resolves to null) instead of recursing without bound.

- **`FallbackNodeCreator` hook** — `runtime/X3DProtoClone.hpp` exposes a process-global `FallbackNodeCreator` function object. When `deepClone` cannot create a node via the generated `X3DNodeFactory`, it tries this hook. `x3d::runtime::ext::install()` populates it so that ext extension nodes (not in the generated factory) can be cloned without modifying the generated layer. The hook is set-once at single-threaded setup time; concurrent writes during cloning are not synchronized.

- **`Scene` expansion tables** — `expandScene` writes to three `Scene` fields that other subsystems consume:
  - `Scene::resolvedProtoRoutes` — body-internal ROUTEs pre-resolved to concrete cloned endpoints; the event cascade (`X3DEventCascade`) picks these up alongside scene-level ROUTEs.
  - `Scene::protoRedirects` — maps each expanded primary node's interface event field to the IS-connected body endpoints; the scene bridge (`X3DSceneBridge`) consults this map when resolving external ROUTEs that target a proto instance.
  - `Scene::expandedSources` — maps each expanded primary node back to its source `ProtoInstance`; codec writers consult this to re-emit `<ProtoInstance>` rather than the cloned body (AUD-B round-trip correctness, commit `8b888ee`).

`Scene::protoPeerNodes` retains non-rendered body peers. Standard runtime systems and the CLI's ScriptSystem enroll them alongside reachable scene nodes; extraction still traverses only rendered roots (§4.4.4.3).

- **`ProtoInstance::expanded` flag** — set to `true` by `expandScene` on a successfully spliced instance. Writers check this flag: if `false`, a parsed instance retains its inert template at its authored node position. Writers use the linked structural record there. Only programmatic instances without a placement template need fallback emission from `Scene::protoInstances`.

- **`ProtoBody::nestedInstances`** — `ProtoInstance` records that appear inside a prototype body are stored here (not in `Scene::protoInstances`) so `expandInstance` recurses per outer instantiation, attaching parent-contained instances to the per-instantiation body clone. Direct body instances participate in primary/peer selection instead of becoming children of the primary.

### Authored body order

`ProtoBody::statements` owns direct nested declarations and records the order of
ordinary nodes and direct instances. `orderedStatements()` reconciles node
occurrences against `nodes`, skips removed nodes and invalid instance indices,
and appends unrecorded programmatic nodes and direct instances. Instance records
use indices into `nestedInstances`; callers that reorder or erase that vector
must update those indices. Repeated node handles preserve USE occurrences.

Expansion selects the first authored node or instance as primary. Later direct
nodes remain active in `Scene::protoPeerNodes`, outside the rendered hierarchy.
A body containing only an instance can expand. If its first instance fails,
expansion does not promote a later peer to change the prototype's type.
Interface event redirects are registered after the primary is known.

XML, canonical XML, Classic and JSON writers preserve direct declaration and
instance ordering, including unused nested declarations and shadowed names.
Each declaration writer shares one DEF/USE context across interface defaults
and body nodes; nested declarations get a fresh context. `ProtoBody::nodeStatements`
records declarations and child occurrences against weak parent identities.
`orderedNodeStatements()` reconciles these with live reflected node fields and
`nestedInstances`: removed occurrences are skipped and new children appended.
Child values follow each current reflected field vector, including reorders and insertions;
declaration and instance entries retain their ledger positions. Its instance indices
have the same caller maintenance requirement as direct records.
Expansion rebuilds affected MFNode slots in this order, preserving USE aliases.

XML and canonical XML replay the merged order. Classic declarations must fall
between complete field assignments; its writer rejects repeated field groups.
JSON likewise rejects repeated slot groups rather than emitting duplicate keys.
These are current implementation limits. JSON round trips demonstrate repository
compatibility, not ISO encoding conformance. Ordinary interface-default graphs use the same capture and writer context, so
contained declarations and instances survive serialization and expand against
each outer instance's cloned default graph. Direct ProtoInstance defaults use authored `ProtoInstanceTemplate`
nodes in the existing `nodeDefault` vector. Expansion materializes required
templates into the shared clone map before IS forwarding; DEF/USE references
across default fields and body graphs share the resulting primary. Explicit
overrides suppress unused defaults, including their external resolution.
Instances in SFNode/MFNode `fieldValue` graphs use the same template type.
An ephemeral expansion context shares each caller template's primary across
field values and scene-root USE aliases. Ordinary caller nodes retain their
identity. Every parsed scene structural instance occupies a template slot in
`rootNodes` or its ordinary parent's node field. A weak `placementTemplate`
link associates that slot with the authoritative structural record. Expansion
replaces slots in place, preserving authored root and child order, including
repeated USE positions. Removing or reordering slots changes placement without
resurrecting records from the structural list. `hasPlacementTemplate()` detects
an authored identity even after its weak pointer expires; programmatic records
that never had a slot retain append/attach behavior. Unresolved caller templates remain inert and
serializable, and resolution may be retried in a fresh transaction. All four
writers emit DEF/USE for shared expanded primaries. See
[ADR-0056](../decisions/0056-prototype-instance-default-templates.md).
Cross-encoding ordering limits remain open under PROTO-NESTED-WRITE. Source UNIT snapshots
include retained unused nested declarations.

### Nested interface routes

Body-local DEF lookup runs after nested instances expand. Expansion captures each
nested interface's IS targets locally, resolves either or both ROUTE endpoints
through the declared interface, and stores concrete endpoints in
`Scene::resolvedProtoRoutes`. These snapshots keep inner and outer interfaces
separate even when they share a primary node. The outer redirect map replaces
the inner map at that pointer. Nested IS event connections forward their physical
targets into the outer interface map.

`inputOutput` fields accept the source alias `name_changed` and sink alias
`set_name`. Access and field types are checked before adding routes; the scene
bridge checks the physical endpoints too. EXTERNPROTO endpoints use the authored
external interface. Other native primary fields are hidden from external routes,
except inherited `metadata`, which currently uses the primary's storage.

Every declared interface field exists on the instance as a real field with
its initial value, whether or not the body IS-connects it (ISO/IEC 19775-1
§4.4.2.2, §4.4.4.2). Expansion registers each unconnected interface field
(all four access types) on the primary as independent storage in the dynamic-field
store — the same side-table Script author fields use (`runtime/events/DynamicField.hpp`).
The store seeds `initializeOnly`/`inputOutput` fields from the `fieldValue`
override or the interface default, gives `inputOutput` fields a set (echoed as
`_changed`) and a get, `inputOnly` fields a set (route sink), `outputOnly` fields a
get (route source), and `initializeOnly` fields value-only storage. The scene
bridge resolves an external ROUTE naming such a field to this independent endpoint
when no IS redirect exists, instead of rejecting it; an enclosing PROTO's
body-local ROUTEs resolve a nested instance's unconnected interface field the same
way. Unconnected `SFNode`/`MFNode` fields also retain independent state: defaults
reuse the instance clone map (preserving aliases), caller overrides retain identity,
and explicit NULL/empty overrides suppress defaults. ROUTEs retain those node
identities, including subsequent NULL/empty events. This is a state-and-endpoint
contract, not a claim that arbitrary dynamic node graphs participate in every
runtime traversal. Dynamic Script fields are available to physical
ROUTE validation, but expansion's IS lookup still uses static reflection.

### IS-connection and access-type rules

IS-connection forwarding follows ISO/IEC 19775-1 Table 4.4. An `inputOutput` body field may map to any interface access type; all other body fields must map to an interface field of the same access type. Violations produce an `InterfaceMismatch` `ProtoWarning` and are skipped. Field types must match; generated enum storage retains its SFString setter compatibility. Body inputOutput aliases `set_name` and `name_changed` can connect to inputOnly and outputOnly interface fields respectively, including across nested prototypes. Only `initializeOnly` and `inputOutput` connections forward values at expansion time; event-only connections (`inputOnly`/`outputOnly`) are registered as redirect entries in `Scene::protoRedirects` for runtime dispatch.

## How it is tested

The test suite is split between unit tests that exercise `expandInstance`/`expandScene` directly and round-trip tests that confirm the expanded scene survives serialization:

| ctest target (doctest case) | What it covers |
|---|---|
| `x3d_parse_tests` (`proto_clone_test`) | `deepClone` — field-by-field copy, DEF/USE shared-identity preservation, SFNode/MFNode recursion (`runtime/parse/tests/proto_clone_test.cpp`) |
| `x3d_parse_tests` (`proto_expand_test`) | `expandInstance`/`expandScene` — field forwarding, IS-connection wiring, scene-root splice, EXTERN no-op with noop resolver, two independent instances (`runtime/parse/tests/proto_expand_test.cpp`) |
| `x3d_event_scene_bridge` | Parsed XML/Classic nested ROUTEs, both interface endpoints, aliases, IS event forwarding, shared-primary interface isolation, per-instance delivery, metadata and invalid endpoint rejection (`runtime/events/tests/scene_bridge_test.cpp`) |
| `x3d_events_tests` (`proto_interface_state_test`) | Unconnected scalar and node-valued interface fields: independent state + routable endpoints (inputOutput echoes `_changed`), default/`fieldValue` seeding, IS-connected path unchanged, sibling nested instances / shared nested-outer primary / EXTERNPROTO keep independent entries (`runtime/events/tests/proto_interface_state_test.cpp`) |
| `x3d_proto_expand_audit` | Audit suite: recursion-limit guard, Table 4.4 access-type validation, bad-any-cast leniency, empty-body warning, EXTERN unresolved warning, MFNode forwarding, nested body instance expansion (`runtime/parse/tests/proto_expand_audit_test.cpp`) |
| `x3d_proto_front_door` | End-to-end XML parse → `parseDocument` → expansion → scene check (`runtime/parse/tests/proto_front_door_test.cpp`) |
| `x3d_parse_tests` (`proto_nested_body_test`) | Nested `ProtoInstance` inside a body: correct per-instance expansion and attachment (`runtime/parse/tests/proto_nested_body_test.cpp`) |
| `x3d_parse_tests` (`vrml97_proto_test`) | PROTO capture + expansion via the VRML97 reader (`runtime/parse/tests/vrml97_proto_test.cpp`) |
| `x3d_parse_tests` (`asset_proto_resolver_test`) | `protoResolverFrom`: fetch → parse → filter/fragment selection over a fake in-memory `AssetResolver`; `Failed` skip, `Pending` hard error, `urn` via a scheme router (`runtime/parse/tests/asset_proto_resolver_test.cpp`) |
| `x3d_codecs_tests` (`proto_roundtrip_test`) | XML round-trip: parse → expand → re-serialize → reparse (`runtime/codecs/tests/proto_roundtrip_test.cpp`) |
| `x3d_codecs_tests` (`proto_instance_roundtrip_test`) | Un-expanded instance re-emitted via `expandedSources` / `expanded` flag (AUD-B) (`runtime/codecs/tests/proto_instance_roundtrip_test.cpp`) |
| `x3d_codecs_tests` (`nested_protoinstance_roundtrip_test`) | Nested instance round-trip via the SDK façade (`runtime/codecs/tests/nested_protoinstance_roundtrip_test.cpp`) |
| `x3d_codecs_tests` (`proto_writer_parity_test`) | Writer parity across XML/VRML/JSON for proto declarations and instances (`runtime/codecs/tests/proto_writer_parity_test.cpp`) |
| `x3d_codecs_tests` (`xml_proto_capture_test`) | XML reader captures `ProtoDeclaration`, `ExternProtoDeclaration`, and `ProtoInstance` records faithfully (`runtime/codecs/tests/xml_proto_capture_test.cpp`) |

Run the full proto suite: `ctest --preset dev -R "x3d_(parse_tests|codecs_tests|proto_front_door|proto_expand_audit)"`.

## Related specs and ADRs

- [Architecture overview](../architecture.md)
- [Parse readers subsystem](../subsystems/parse-readers.md) — readers that produce the `ProtoInstance` / `ProtoDeclaration` records consumed by this subsystem
- [Scene graph subsystem](../subsystems/scene-graph.md) — `Scene` fields written by expansion (`resolvedProtoRoutes`, `protoRedirects`, `expandedSources`)
- [Routes subsystem](../subsystems/routes.md) — how `resolvedProtoRoutes` integrates with the event graph
- [Execution context subsystem](../subsystems/execution-context.md) — `X3DSceneBridge` consults `protoRedirects` when resolving external ROUTEs targeting proto instances
- Normative reference: ISO/IEC 19775-1, §4.4 (PROTO semantics), Table 4.4 (IS access-type constraints) — cited throughout `runtime/X3DProtoExpand.hpp` inline comments
- AUD-B round-trip fix (commit `8b888ee`): `ProtoInstance::expanded` flag + `Scene::expandedSources` writer redirect — see `docs/superpowers/BACKLOG.md` (deprecated, historical)
