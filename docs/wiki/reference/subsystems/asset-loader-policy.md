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
