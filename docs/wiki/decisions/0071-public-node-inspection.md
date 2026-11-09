---
title: "ADR-0071: Public node inspection"
summary: Viewer discovery through an identity walk and owned typed field reads.
tags: [adr, sdk, reflection]
updated: 2026-10-18
related:
  - ../subsystems/sdk-facade.md
---

# ADR-0071: Public node inspection

## Status

Proposed. Compilation and test execution await the maintainer.

## Problem

CAVEOS needs viewpoint discovery, labels, and NavigationInfo.headlight, but
names `runtime::detail::forEachNode` and `runtime::geombounds::getField` to
obtain them. On the round-two host these occur in
`caveos/plugins/x3d/x3d_scene.cpp:150`, `x3d_app.cpp:275,283`, and
`x3d_vsg.cpp:241`. The original host also uses `runtime::FieldRef<MFString>`
in `x3d_scene.cpp:88` to read URL fields before rewriting them.

## Decision

Add experimental templates directly to `x3d/sdk.hpp`:

```cpp
sdk::forEachNode(scene, [&](sdk::X3DNode *node) { /* discover */ });
auto description = sdk::readField<x3d::core::SFString>(*view, "description")
                       .value_or("");
bool headlight = sdk::readField<x3d::core::SFBool>(*nav, "headlight")
                     .value_or(true);
```

Reuse the runtime's existing identity walk. Visit roots, then PROTO peers,
depth first before children, following generated SFNode/MFNode slots in
reflection order. Include inactive branches and already loaded Inline children;
skip nulls and repeat identities. Do not load resources, descend uninstantiated
declarations, or resolve author-declared Script fields. Callbacks borrow node
pointers and must not mutate the graph during traversal.

Read generated reflected getters by exact name and C++ type into an owned
optional value. Missing/unreadable fields and type mismatches return `nullopt`;
there is no debug-only assertion, conversion, event alias resolution, or
exception suppression. Callers choose their fallback. Author fields continue
to require the context-aware Script/SAI APIs.

## Consequences

Existing callers are unchanged. The host can replace the private walk calls
with `sdk::forEachNode`, and bounds-helper reads with the optional reads above.
The original host's MFString read can use
`sdk::readField<x3d::core::MFString>(*node, field.x3dName.c_str())`; its URL
rewrite and writes are separate concerns, already removed by round one's
base-aware resolvers. Gzip loading remains outside this proposal.

Reads copy values, including vectors. This deliberately avoids exposing the
borrow-or-box lifetime of `FieldRef` for occasional viewer inspection. A walk
is not a render-path walk and does not report USE placements separately.

The registered `x3d_sdk_node_inspection` test includes only `x3d/sdk.hpp` for
SDK APIs. It checks SFNode/MFNode descent, inactive branches, peers, USE sharing,
cycles, nulls, empty scenes, typed labels and booleans, missing names, wrong
types, alias rejection, and ownership after a later field write. Without this
proposal the public API names fail to compile. No build or test was run.

## Alternatives

Re-exporting `FieldRef` exposes borrowed storage and fallback boxing to the
host. Re-exporting `geombounds::getField` carries debug assertions for wrong
types and requires a fallback even when the caller wants to detect absence.
Context-aware author-field discovery would widen this proposal beyond the
viewer workaround.
