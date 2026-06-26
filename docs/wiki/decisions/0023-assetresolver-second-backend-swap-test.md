---
title: "ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Behavioral Swap-Test as the Genericity Proof"
summary: An interface is *proven generic* only when a second, independent backend runs identical fixtures to identical observable behavior, gated in CI — applied to the AssetResolver/IO seam by adding an S3 SDK backend alongside the libcurl HTTP backend and a CI-gated swap-test. The pattern from ADR-0022; the highest-leverage IO seam because it unblocks LoadSensor, http/urn EXTERNPROTO, Script external-URL, and autoRefresh.
tags: [adr, seam, genericity, assetresolver, io, libcurl, s3, swap-test, ci-gate, thesis]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../seam-status.md
  - 0022-scriptengine-second-backend-swap-test.md
  - 0019-physics-seam.md
---

# ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Behavioral Swap-Test as the Genericity Proof

## Status

Accepted (proposed 2026-06-23; becomes Implemented after the swap-test job turns green).

## Context

The product thesis is that x3d-cpp is **unopinionated and pluggable**: every place a
renderer/engine plugs in is an abstract **seam**, and the runtime core stays spec-correct and
backend-free. The `AssetResolver` seam is the **bytes path**: how a CONSUMER turns a URL surfaced
verbatim in `TextureRef` into the raw bytes it then decodes. Today the only concrete backend is
the PoC's `makeLocalFileResolver` (`examples/poc_renderer/main.cpp:394`); the only embedder-callback
seam in production code is the `std::function<AssetResult(url, AssetKind)>` type itself in
`runtime/extract/AssetResolver.hpp`.

That is one implementation — and one is not a proof. The interface could be quietly shaped by the
local-file path (e.g. assuming a scene-dir baseUrl) without the seam ever saying so. The way to
find out is to make a **second, independent** backend satisfy the same interface — one that uses a
genuinely different bytes-fetching model — and prove the two are interchangeable by **behavior**,
not by inspection.

This is the second seam to take that proof after [ADR-0022](0022-scriptengine-second-backend-swap-test.md)
(ScriptEngine: Duktape + QuickJS). AssetResolver/IO is the **highest-leverage** next seam because
shipping it unblocks several P1 dependency cards:

- **NSN-*** (LoadSensor) — currently inert; would call the HTTP backend at runtime.
- **PRF-6** (http/urn EXTERNPROTO) — the parse-time PROTO seam (`ProtoDeclarationResolver`)
  unblocks separately, but its first useful target is an http:// URL fetched by an HTTP backend.
- **CONF-CRITIC-2** (Script external-URL) — scripts with `@url` need a fetch path.
- **SCR-005** (autoRefresh) — periodic re-fetch.

The two backends chosen are **libcurl HTTP** and the **AWS C++ SDK (s3)** — they share zero
implementation (libcurl is C with a sync I/O loop; aws-sdk-cpp is C++ with a coroutine-capable
async client and a fully-typed S3 protocol), so a parity failure would surface a real interface
leak, not a coincidence.

## Decision

**An interface is _proven generic_ only when a second independent backend runs identical fixtures
to identical observable behavior, gated in CI** (the ADR-0022 rule, restated for this seam).

Concretely, for the `AssetResolver` pilot:

- **Backend A — libcurl HTTP** (`runtime/io/curl/HttpResolver.{hpp,cpp}`), behind the
  `X3D_CPP_BUILD_CURL` build option, with **no core `#ifdef`** — libcurl meets the seam in a
  single isolated TU (`x3d_curl` static lib, linked PRIVATE so its flags/headers never leak to
  consumers). The default build (option OFF) is byte-identical and behavior-unchanged. Sync
  `curl_easy_perform`; deferred-bytes Pending is a follow-up.
- **Backend B — AWS C++ SDK S3** (`runtime/io/s3/S3Resolver.{hpp,cpp}`), behind
  `X3D_CPP_BUILD_S3`, with **no core `#ifdef`** — aws-sdk-cpp meets the seam in a single isolated
  TU (`x3d_s3` static lib, linked PRIVATE). The default build (option OFF) is byte-identical.
  Maps `s3://bucket/key` URLs to `s3::GetObject` calls.
- **Behavioral swap-test** (`x3d_assetresolver_swap`): a ctest binary drives the *same* fixture
  bytes through Backend A (against an in-process HTTP server in the test binary) and Backend B
  (against a docker minio fixture) and asserts **byte-equal `AssetResult.bytes`** for every
  fixture, plus equal `AssetStatus::Failed` for missing keys. Equality is observable
  (the bytes the resolver hands back), never internal — so per-backend transport details stay
  inside each backend.
- **Permanent CI merge gate**: `AssetResolver seam swap-test` job in
  `.github/workflows/ci.yml`, flag-gated `-DX3D_CPP_BUILD_CURL=ON -DX3D_CPP_BUILD_S3=ON`,
  docker minio service, `ctest -R 'x3d_assetresolver(_backend|_swap)'`.
- Because the interface carried two backends with no signature change, it is promoted
  `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp:131` — the whole `AssetResolver` /
  `AssetResult` / `AssetKind` surface as one frozen seam.

## Consequences

**Positive:**

- The genericity claim for `AssetResolver/IO` is now **empirical**, not aspirational — and the
  permanent CI gate keeps it true.
- **Four P1 dependency cards unblock**: LoadSensor (NSN-*), http/urn EXTERNPROTO (PRF-6), Script
  external-URL (CONF-CRITIC-2), autoRefresh (SCR-005) — each can now build on top of an
  interface that is *proven* generic rather than *claimed* generic.
- The PRF-6 parse-time seam (`ProtoDeclarationResolver`) follows the same pattern; the
  http/urn EXTERNPROTO card picks it up.
- A genericity *leak* becomes a **finding**, not a silent risk: if the S3 SDK ever forces an
  `AssetResolver` signature change, the swap-test fails loudly.

**Trade-offs / costs:**

- AWS C++ SDK is a real ongoing dependency: `find_package(AWSSDK REQUIRED COMPONENTS s3)` when
  ON; a heavy compile behind the OFF-default option keeps the default PR path cold-safe.
- Docker in CI: the swap-test job needs a `services:` minio container. Documented; locally,
  `docker run -d -p 9000:9000 -p 9001:9001 minio/minio server /data`.
- Two bytes-fetching implementations must stay in lockstep on URL-subset semantics
  (libcurl honors `http(s)://`; S3 honors `s3://` + virtual-hosted) — though the swap-test makes
  divergence a loud CI failure.
- The proof bar is now higher for *every* remaining seam: Physics (Jolt + Bullet), Audio
  (miniaudio + OpenAL), FontMetrics (FreeType + stb_truetype), TextureResolver (stb_image +
  libpng), GeoProjection (PROJ + WGS84), Consumer (OpenGL PoC + Godot). That is the intended cost
  — it is the whole point of the thesis.

## Related

- [Seam-Status Matrix](../seam-status.md) — the live tracker this ADR establishes the rule for.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Behavioral Swap-Test](0022-scriptengine-second-backend-swap-test.md)
  — the pattern this card applies to the IO seam.
- [ADR-0019: Physics via a Flag-Gated Engine Backend](0019-physics-seam.md) — the flag-gated
  isolated-backend precedent (Jolt) this build isolation mirrors.
- Design spec: `docs/superpowers/specs/2026-06-23-assetresolver-http-design.md`.
