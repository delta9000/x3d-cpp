---
title: Asset loader policy
summary: How recursive Inline and EXTERNPROTO loading is confined, bounded and reported at parse time.
tags: [subsystem, parse, security, inline, externproto]
updated: 2026-11-14
related:
  - parse-readers.md
  - system-asset-io.md
---

# Asset loader policy

The readers that apply this policy are in [Parse readers](parse-readers.md);
byte loading is in [Asset IO](system-asset-io.md).

- `assetResolversFrom(AssetResolver, Encoding)` in
  `runtime/parse/AssetDocumentResolvers.hpp` returns `.proto` and
  `.inlineScene`.
- Pass both to `parseDocument()` to retain one synchronous asset policy across
  every Inline/EXTERNPROTO document boundary.
- Nested parsing never enables the default local-file loader.
- Backends receive fragment-free URLs resolved against the source document's
  directory, including nested relative references.
- The helper normalizes dot segments for cycle identity while preserving the
  authority, query/fragment bytes, opaque identifiers and significant empty
  path segments.
- Terminal `/.` and `/..` retain directory semantics (including authority and
  relative roots), so documents fetched at directory URLs resolve their nested
  references against that same directory.
- It performs no filesystem or network I/O itself.

```cpp
const auto loaders = x3d::codec::assetResolversFrom(myAssets);
auto doc = x3d::codec::parseDocument(text, encoding, sourceDirectory,
                                    loaders.proto, loaders.inlineScene);
```

- `protoResolverFrom(asset, hint)` remains available in
  `AssetProtoResolver.hpp`; its nested Inline loads now use the same asset
  backend rather than an implicit local-file default.
- `inlineResolverFrom(asset, hint)` in `AssetInlineResolver.hpp` supplies the
  symmetric loader.
- To retain an independent opposite policy, use `protoResolverFrom(asset,
  hint, inlinePolicy)` or `inlineResolverFrom(asset, hint, protoPolicy)`.
- An explicitly empty opposite callback denies those loads.
- Independently supplied callbacks are opaque: pass the intended override to
  the helper as well as the top-level parser, or use the paired factory.
- No global or thread-local loader policy is installed.
- Failed/unparseable candidates remain lenient and try the next URL.
- A Pending asset result aborts the entire active recursive asset load,
  including parent candidate lists; the parser records its ordinary unresolved
  Inline/EXTERNPROTO warning.
- Backend exceptions are treated as failed candidates.
- Cross-kind cycles share an explicit active-URL stack; independent top-level
  calls start clean.
- Asset-loaded Inline nodes retain a value-owned loader seeded with their
  source ancestry.
- Deferred `load=TRUE` events therefore reject self and mutual cycles across
  ticks, including lexical dot-segment aliases.
- Siblings and reloads remain independent.
- This retained policy takes precedence over a session's fallback Inline
  resolver for those nodes; choose the asset policy when loading the world.
- Custom resolvers and the local-file loader do not acquire source identity
  from this helper.
- See
  [ADR-0070](../../explanation/decisions/0070-deferred-inline-loader-ancestry.md).
- The additive three-argument overload accepts a `ProtoResolverDecorator`:
  `assetResolversFrom(assets, hint, [](auto base) { return decorate(base);
  })`.
- It wraps the asset-backed PROTO resolver at every parse boundary and also
  wraps the returned top-level `.proto` callback.
- The decorator must delegate to its supplied callback to retain the loader's
  confinement, Pending propagation, and cycle checks.
- Its base callback borrows synchronous invocation state: don't retain it
  outside the invocation.
- The factory retains the decorator by value; its captures must outlive use of
  the loaded world.

`x3d_ext_cave_recursive_resolver` exercises ext declarations in nested files.
The `x3d_extract_tests` case
`Session: deferred asset Inline cycles retain source ancestry` checks self and
mutual cycles, lexical aliases, independent siblings, and reload after unload.
Both additions await maintainer execution.

- The front door also captures source-directory provenance separately from
  serialized values.
- `ProtoDeclaration::sourceBaseUrl` and
  `ExternProtoDeclaration::sourceBaseUrl` survive cached/fetched declarations;
  body/default clones retain that directory in `Scene::nodeBaseUrls`.
- URL values forwarded by IS retain the supplying fieldValue's source
  directory.
- Consequently an imported prototype's body URLs resolve against its library
  while an authored URL override resolves against the caller.
- Authored URL strings are unchanged.
- The local-file Inline resolver uses the same confinement/canonicalization
  helper as EXTERNPROTO; rejected absolute/escaping includes never open the
  target.

The front door snapshots each document's authored `head.units` onto its `Scene::sourceUnits` and locally authored `ProtoDeclaration::sourceUnits` before expansion. Child Scenes returned by Inline and declarations selected by file or asset EXTERNPROTO resolution therefore carry their own source UNIT declarations; an empty snapshot means that source declared none. These runtime snapshots do not change `Head` serialization or numeric field values.

- Dialect hooks on `ClassicVrmlReader` — three protected virtual methods that `Vrml97Reader` overrides:
  - `mapFieldName(nodeType, token)` — renames a field token (identity in Classic VRML; applies the LOD/Switch field renames in VRML97).
  - `onHeaderLine(parts, doc)` — reuses the first line's tokens to set `doc.version`, `doc.profile`, or reject an unsupported encoding (e.g. VRML 1.0 throws in `Vrml97Reader`).
  - `warn(message)` — diagnostic sink; discarded by the base, collected into a `std::vector<std::string>` by `Vrml97Reader` (optional strict mode via `setStrict(true)`).

ClassicVRML extracts and tokenizes the header once, then shares the tokens
with the VRML97 dialect hook. Node type names go directly to the factory;
VRML97 remaps only field names.

JSON document reads require exactly one JSON value. Trailing whitespace is
accepted; trailing garbage or another JSON value throws. The regression case
`json_document_rejects_trailing_content` exercises JsonLite, JsonReader, and
`parseDocument()`.

- Node factory — all readers instantiate nodes via `X3DNodeFactory::create(typeName)`, which is the same factory used everywhere else in the runtime. Unknown type names return null, and the reader records a `ReaderWarning{Kind::UnknownNode}` in `X3DDocument.readerWarnings` so `x3d validate` reports it (DIAG-UNKNOWN-NODE). The XML reader then keeps the element as an opaque `runtime::UnknownNode` (`runtime/X3DProto.hpp`): type name, DEF, authored attributes as wire text in source order, character data, and child nodes, which are read normally so their DEFs resolve. It sits in the parent's named slot or `children` field; when the parent has neither (a 4.1 `<Tangent>` under a 4.0 `IndexedFaceSet`) it is parked in `Scene::opaqueChildren`. An `UnknownNode` has an empty field table, so the runtime, the extractor and PROTO cloning ignore it, and a PROTO instance loses it on expansion. `<field>`, `<IS>` and PROTO statements inside an unknown element are discarded with a warning. The ClassicVRML, VRML97 and JSON readers still skip an unknown node (READ-UNKNOWN-NODE-PASSTHROUGH). In XML, a `<ROUTE>` nested among a node's children is not a node: it is added to the enclosing scene's (or PROTO body's) routes, and a nested `<IS>` is skipped there because the IS pass reads it.

- Reader-recovery diagnostics — `X3DDocument.readerWarnings` (`runtime/X3DProto.hpp`) is the structured channel for lenient-read recoveries that are not legal X3D: an unknown/misspelled node element (`UnknownNode`) and an unknown `profile=` token coerced to `Interchange` (`ProfileCoerced`, DIAG-PROFILE-COERCE). Every reader (`XmlReader`, `ClassicVrmlReader`/`Vrml97Reader`, `JsonReader`) fills it; `cmdValidate` and the cli-gate validate path surface each as a `node`/`profile` diagnostic. It parallels `rangeWarnings`/`protoWarnings`/`inlineWarnings`. A third kind, `Value`, records an authored value that `parseValue` coerced: an unparsable number token (read as 0), a short SF tuple (padded with 0), or a trailing partial MF tuple (dropped). Readers call `noteValueSyntax` for node fields and ProtoInstance fieldValues; `parseDocument` collects them through a thread-local `ValueDiagnosticScope` (`runtime/codecs/FieldValueIO.hpp`) and appends them after the read (VALIDATE-VALUE-SYNTAX).

- Profile token preservation — `X3DDocument::setProfileToken` records the authored `profile=` spelling verbatim in `profileRaw` (exposed via `profileToken()`), resolves it to a `Profile` for profile-fit, and diagnoses a non-canonical token. The XML/JSON/VRML writers emit `profileToken()`, so a round-trip never rewrites the declared conformance class (DIAG-PROFILE-COERCE).

- UNIT header validation — `parseDocument()` checks UNIT declarations before scene expansion for all supported X3D encodings. Declarations require X3D 3.3 or later, one of `angle`/`force`/`length`/`mass`, no duplicate category, a nonempty name without whitespace, and a finite positive conversion factor. An invalid declaration throws; valid authored factors and field values remain unchanged for round-tripping. Runtime conversion is described by `runtime/UnitConversion.hpp`. Runtime entry converts explicitly authored `TextureTransform.rotation` through the angle UNIT factor, including values forwarded via IS; UV center, translation and scale remain dimensionless. The focused `UNIT runtime: TextureTransform*` cases in `runtime_session_test.cpp` cover defaults, repeated initialization and subsequent radian writes.

- PROTO built-in-shadow quarantine — after `readDocument`, `parseDocument()` runs `quarantineBuiltinShadowingProtos` (`runtime/parse/X3DParse.cpp`): any `ProtoDeclare`/`ExternProtoDeclare` whose name is a built-in (`X3DNodeFactory::registry()`) is dropped and recorded as `ProtoWarning{Kind::BuiltinShadow}`, so the built-in keeps precedence (ADR-0033, `PROTO-SHADOW`). Applies uniformly across all four encodings from the single front door; lenient by default.

- Reflection / field population — all readers set fields through the `FieldInfo` thunks exposed by `node.fields()` (the reflection `FieldTable`). `build::applyField` routes enum fields through `setEnumString` and everything else through `FieldValueIO::parseValue + set`. The `outputOnly`/`inputOnly` access guards in `applyField` skip read-only fields during parse.

- DynamicFieldStore (S1 seam) — `ClassicVrmlReader`, `JsonReader`, and `XmlReader` capture author `<field>` declarations into the current `Scene::authorFields` owner as `AuthorFieldDecl` entries (see `runtime/events/DynamicField.hpp`). Script fields are captured across the supported readers; XML, Classic, and JSON also capture programmable-shader author declarations such as `ComposedShader` uniforms, including node-valued defaults. Declaration-local Scenes transfer their owner to `ProtoDeclaration::authorFields`; callers do not depend on a process-global table. This seam is the parse-reader touchpoint for the Script/SAI and author-shader runtimes; all other nodes are fully handled by the reflection layer. Inline `<![CDATA[...]]>` source is mirrored into the node's `sourceCode` slot — `Script.sourceCode` for scripts, `ShaderPart.sourceCode` for a ComposedShader's GLSL stages and `ShaderProgram.sourceCode` for a ProgramShader's — so the runtime has a uniform source path.

- PROTO body order — direct body declarations, nodes and instances are recorded in `ProtoBody::statements`. The statement records retain unused local/extern declarations and the point at which a nested declaration shadows an inherited name. Node and instance values remain in the existing body collections. Ordinary body-node children and declarations are also recorded in `ProtoBody::nodeStatements`, keyed by weak parent identity. Reflected fields and `nestedInstances` remain the value authority. Ordinary interface-default node graphs share this context. Direct ProtoInstance defaults occupy `nodeDefault` as authored `ProtoInstanceTemplate` nodes. The local DEF table resolves USE references to the same template across fields and body graphs; these defaults do not become direct body instances. XML, Classic and JSON also capture direct and contained ProtoInstances in SFNode/MFNode fieldValues as bound templates in the owning value graph. All scene structural instances, named or unnamed, occupy an authored template slot in `rootNodes` or their ordinary parent's node field. The structural record's weak `placementTemplate` link preserves the exact identity; expansion replaces the slot without appending it after ordinary nodes. USE references reuse the same expanded primary. Raw writers read source values from the linked structural record while retaining slot order.

- PROTO DEF scoping — XML, JSON, and Classic readers parse each declaration's interface defaults and body through one local `Scene`. DEF/USE aliases therefore resolve across fields and into the body, while outer scene DEFs and nested PROTO DEFs remain separate (§4.4.4.4). Previously declared PROTO/EXTERNPROTO declarations remain available for nested instances; a same-name nested declaration shadows the inherited name across both declaration kinds without changing the enclosing scene. The declaration carries authored scalar-field marks from both interface and body after the local scene is consumed. IS-connection links are threaded through as `runtime::IsConnection` entries on `runtime::ProtoBody`.
