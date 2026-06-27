---
title: "PROTO/EXTERNPROTO Expansion"
summary: Clones and splices PROTO/EXTERNPROTO instances into the live scene graph after parsing, forwarding field values and wiring IS-connections.
tags: [subsystem, proto, externproto, expansion, is-connection]
updated: 2026-06-24
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
| `runtime/parse/X3DParse.hpp` | Front door `parseDocument` — invokes `expandScene` after the reader and range-warning passes; defines `localFileProtoResolver` (file-local default) |
| `runtime/X3DScene.hpp` | `Scene` — owns `protoInstances`, `resolvedProtoRoutes`, `protoRedirects`, `expandedSources`, `protoDeclarations`, `externProtoDeclarations` |

## Interfaces and seams

### Exposed interface

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

- **`FallbackNodeCreator` hook** — `runtime/X3DProtoClone.hpp` exposes a process-global `FallbackNodeCreator` function object. When `deepClone` cannot create a node via the generated `X3DNodeFactory`, it tries this hook. `x3d::runtime::ext::install()` populates it so that ext extension nodes (not in the generated factory) can be cloned without modifying the generated layer. The hook is set-once at single-threaded setup time; concurrent writes during cloning are not synchronized.

- **`Scene` expansion tables** — `expandScene` writes to three `Scene` fields that other subsystems consume:
  - `Scene::resolvedProtoRoutes` — body-internal ROUTEs pre-resolved to concrete cloned endpoints; the event cascade (`X3DEventCascade`) picks these up alongside scene-level ROUTEs.
  - `Scene::protoRedirects` — maps each expanded primary node's interface event field to the IS-connected body endpoints; the scene bridge (`X3DSceneBridge`) consults this map when resolving external ROUTEs that target a proto instance.
  - `Scene::expandedSources` — maps each expanded primary node back to its source `ProtoInstance`; codec writers consult this to re-emit `<ProtoInstance>` rather than the cloned body (AUD-B round-trip correctness, commit `8b888ee`).

- **`ProtoInstance::expanded` flag** — set to `true` by `expandScene` on a successfully spliced instance. Writers check this flag: if `false`, the instance failed to expand and must be re-emitted directly from `Scene::protoInstances` rather than be silently dropped.

- **`ProtoBody::nestedInstances`** — `ProtoInstance` records that appear inside a prototype body are stored here (not in `Scene::protoInstances`) so `expandInstance` recurses per outer instantiation, attaching each nested primary to the per-instantiation body clone rather than to the un-cloned template.

### IS-connection and access-type rules

IS-connection forwarding follows ISO/IEC 19775-1 Table 4.4. An `inputOutput` body field may map to any interface access type; all other body fields must map to an interface field of the same access type. Violations produce an `InterfaceMismatch` `ProtoWarning` and are skipped. Only `initializeOnly` and `inputOutput` connections forward values at expansion time; event-only connections (`inputOnly`/`outputOnly`) are registered as redirect entries in `Scene::protoRedirects` for runtime dispatch.

## How it is tested

The test suite is split between unit tests that exercise `expandInstance`/`expandScene` directly and round-trip tests that confirm the expanded scene survives serialization:

| ctest target (doctest case) | What it covers |
|---|---|
| `x3d_parse_tests` (`proto_clone_test`) | `deepClone` — field-by-field copy, DEF/USE shared-identity preservation, SFNode/MFNode recursion (`runtime/parse/tests/proto_clone_test.cpp`) |
| `x3d_parse_tests` (`proto_expand_test`) | `expandInstance`/`expandScene` — field forwarding, IS-connection wiring, scene-root splice, EXTERN no-op with noop resolver, two independent instances (`runtime/parse/tests/proto_expand_test.cpp`) |
| `x3d_proto_expand_audit` | Audit suite: recursion-limit guard, Table 4.4 access-type validation, bad-any-cast leniency, empty-body warning, EXTERN unresolved warning, MFNode forwarding, nested body instance expansion (`runtime/parse/tests/proto_expand_audit_test.cpp`) |
| `x3d_proto_front_door` | End-to-end XML parse → `parseDocument` → expansion → scene check (`runtime/parse/tests/proto_front_door_test.cpp`) |
| `x3d_parse_tests` (`proto_nested_body_test`) | Nested `ProtoInstance` inside a body: correct per-instance expansion and attachment (`runtime/parse/tests/proto_nested_body_test.cpp`) |
| `x3d_parse_tests` (`vrml97_proto_test`) | PROTO capture + expansion via the VRML97 reader (`runtime/parse/tests/vrml97_proto_test.cpp`) |
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
