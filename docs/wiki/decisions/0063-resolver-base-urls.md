---
title: "ADR-0063: Resolver base URLs"
summary: Add optional contextual callbacks without changing stable resolver aliases or authored URL fields.
tags: [adr, textures, io, sdk]
updated: 2026-10-18
related:
  - ../subsystems/extract-textures.md
  - ../subsystems/system-asset-io.md
  - ../seam-status.md
---

# ADR-0063: Resolver base URLs

Proposed. CAVEOS currently rewrites URL fields to absolute paths before creating
its session and again for runtime Inline children (`x3d_scene.cpp:58,179,198`).
A one-argument texture callback cannot distinguish identical relative URLs from
different source scenes, although the scene already retains that provenance.

Keep `TextureResolver` and `AssetResolver` unchanged. Add optional experimental
`TextureResolverWithBase(url, baseUrl)` and
`AssetResolverWithBase(url, kind, baseUrl)` callbacks, exposed through session
options. A supplied contextual callback takes precedence over its legacy peer.
The texture callback runs wherever extraction already resolves still images;
the asset callback runs in LoadSensor, using the watched child's base.

Pass the authored URL unchanged. Use the originating node's
`Scene::nodeBaseUrls` entry, including an explicitly empty entry; only an absent
entry uses `SessionOptions::baseUrl` (or the low-level configured fallback).
Look up provenance during extraction/polling so runtime Inline additions work.
The host owns URL joining, I/O, and confinement. No path is made safe merely by
being paired with a base.

Texture refs retain weak source-node identity and expose `baseUrl`, including
nested stages, cube faces, movie descriptors, and background panorama faces.
Ready texture memoization with a contextual callback uses `(baseUrl, url)`;
LoadSensor also includes asset kind. Legacy memo keys keep their existing
behavior. Pending and Failed are not memoized; MFString ordering is unchanged.

Movie decoding is host-driven through `MovieDecoder`; it has no session-owned
fetch loop. Hosts can fetch movie bytes using the extracted ref's base without
rewriting the authored URL. Parse-time asset loaders already resolve URLs
against the source directory before calling `AssetResolver`, so their callback
contract stays unchanged. Other asset consumers (audio, shaders, external
geometry) are outside this proposal; this change does not claim to replace
CAVEOS normalization for every URL-bearing node.

This preserves source compatibility for existing callback assignments and
ordinary constructor calls. Added option/descriptor members and changed inline
function signatures do not promise binary compatibility; consumers must rebuild.
The new callbacks have not inherited the stable aliases' backend swap proof.
Regression cases are registered in `x3d_extract_tests` via
`runtime/extract/tests/resolver_base_test.cpp`; build and execution are deferred
to the maintainer for this proposal.
