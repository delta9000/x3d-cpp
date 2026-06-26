---
title: Asset Resolver / IO Seam
summary: The bytes path from a URL surfaced verbatim in `TextureRef` to the raw bytes a consumer decodes — one callback type, two invocation contracts, two proven-generic backends (libcurl HTTP + AWS S3 SDK). The seam that unblocks LoadSensor, http/urn EXTERNPROTO, Script external-URL, and autoRefresh.
tags: [subsystem, seam, asset-resolver, io, libcurl, s3, genericity, proven]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../seam-status.md
  - ../decisions/0023-assetresolver-second-backend-swap-test.md
  - ../decisions/0022-scriptengine-second-backend-swap-test.md
  - ../subsystems/extract.md
  - ../subsystems/ext-firewall.md
---

# Asset Resolver / IO Seam

The `AssetResolver` seam is the **bytes path** — the place where a CONSUMER turns a URL
surfaced verbatim in `TextureRef` (or in an Inline / EXTERNPROTO / ExternalGeometry
descriptor) into the raw bytes it then decodes. The SDK never does I/O; the embedder
supplies a `std::function<AssetResult(url, AssetKind)>` callback and a CONSUMER calls
it. **Two independent backends** (`HttpResolver` over libcurl, `S3Resolver` over the
AWS C++ SDK) plus a CI-gated swap-test prove the seam is generic ([ADR-0023](../decisions/0023-assetresolver-second-backend-swap-test.md));
the interface is frozen `[STABLE]` in [`include/x3d/sdk.hpp:50`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp).

## Purpose

Answer one question per asset reference: "given a URL and an `AssetKind`, what are the
raw bytes?" The seam is intentionally minimal — the callback signature is a single
`std::function`, the result is `{ status, bytes }`, and the SDK threads nothing but
bytes back to the consumer. **Decoding** (PNG / JPEG / etc.) is the consumer's job; the
seam is purely about bytes-on-the-wire.

## The seam (the only contract)

```cpp
// runtime/extract/AssetResolver.hpp — header-only, std-only, no node/transform deps.
namespace x3d::runtime::extract {

enum class AssetKind    { Texture, Movie, Inline, ExternProto, ExternalGeometry };
enum class AssetStatus { Ready, Pending, Failed };

struct AssetResult {
  AssetStatus status = AssetStatus::Failed;
  std::vector<std::uint8_t> bytes;
  static AssetResult makeReady(std::vector<std::uint8_t>);
  static AssetResult makePending();
  static AssetResult makeFailed();
};

using AssetResolver =
    std::function<AssetResult(const std::string &url, AssetKind kind)>;

}  // namespace x3d::runtime::extract
```

### One type, two invocation contracts

The SAME `AssetResolver` signature serves two distinct call sites with DIFFERENT
lifetimes — locked at B8 design, not papered over:

| Contract | Call site | AssetKind | `Pending` legal? | When called |
|---|---|---|---|---|
| **(A) Render-time** | Texture / Movie bytes in the consumer's render loop, AFTER extraction | `Texture`, `Movie`, `ExternalGeometry` | **Yes** — retry next frame, never blocks a frame | Per `TextureRef` with `Source::Url` |
| **(B) Parse-time** | Inline / EXTERNPROTO at scene-build, BEFORE `buildSceneGraph` | `Inline`, `ExternProto` | **No** — must return `Ready`/`Failed` synchronously (Pending is incoherent: nodes must exist before extraction can walk them) | During `expandScene` / `expandInlines` |

A single embedder resolver can branch on `AssetKind` to honor both contracts; both
backends (`HttpResolver`, `S3Resolver`) return `Failed` (never `Pending`) in v1 —
deferred-bytes Pending is a documented follow-up.

## Backends (both proven generic)

Both backends implement the unchanged `AssetResolver` type behind their own CMake option
(mirroring the `x3d_quickjs` / `x3d_physics_jolt` isolation discipline: option →
find_package → an isolated STATIC lib whose single TU is the resolver, with the
heavyweight SDK linked PRIVATE so its headers/flags never leak to consumers).

### Backend A — libcurl HTTP (`x3d_curl`, `X3D_CPP_BUILD_CURL`)

`runtime/io/curl/HttpResolver.{hpp,cpp}`:

- **Transport**: synchronous `curl_easy_perform` per call.
- **URL subset**: `http://` and `https://` only; returns `Failed` for `urn:`, `file:`,
  missing scheme.
- **Status mapping**: `CURLE_OK` + 2xx → `Ready(bytes)`; `CURLE_*` or non-2xx → `Failed`.
- **Build option**: `-DX3D_CPP_BUILD_CURL=ON` (OFF default, `find_package(CURL REQUIRED)`).
  Default build (option OFF) is byte-identical and behavior-unchanged.
- **Per-backend test**: `x3d_assetresolver_backend_a` (URL prefix check + libcurl
  error path against unreachable host). Success path is in the swap-test.

### Backend B — AWS C++ SDK S3 (`x3d_s3`, `X3D_CPP_BUILD_S3`)

`runtime/io/s3/S3Resolver.{hpp,cpp}`:

- **Transport**: `Aws::S3::S3Client::GetObject` per call.
- **URL subset**: `s3://<bucket>/<key>[?...]` only; returns `Failed` otherwise. The
  `endpoint` parameter (default `http://localhost:9000`) overrides the SDK's endpoint
  resolver for S3-compatible fixtures (docker minio in CI).
- **SDK init**: lazy `Aws::InitAPI` via `std::once_flag` on first call. Embedder does
  not need to call `Aws::InitAPI` / `ShutdownAPI` explicitly (idempotent if they do).
- **Build option**: `-DX3D_CPP_BUILD_S3=ON` (OFF default, `find_package(AWSSDK REQUIRED
  COMPONENTS s3)`). Default build (option OFF) is byte-identical.
- **Per-backend test**: `x3d_assetresolver_backend_b` (URL prefix check + `s3://`
  parse + SDK init path against unreachable endpoint). Success path is in the swap-test.

## Genericity proof — `x3d_assetresolver_swap`

`runtime/io/tests/asset_resolver_swap_test.cpp` — a ctest binary that drives identical
fixture bytes through Backend A (libcurl HTTP) and Backend B (AWS S3) and asserts:

- **Byte equality**: `resultA.bytes == resultB.bytes` (size + content via `std::equal`)
  for every fixture.
- **Failure parity**: `resultA.failed() == resultB.failed()` and both have empty
  `bytes` for missing keys.
- **Hermetic**: in-process POSIX-socket HTTP server on 127.0.0.1 (no external HTTP
  server needed); fixtures uploaded to minio via the AWS SDK at swap-test setup.
- **Skip-when-no-minio**: when `$X3D_S3_ENDPOINT` is unset (no docker), the test
  degrades to an HTTP-only parity check and emits a SKIP message — so local dev
  without docker doesn't get a spurious failure.

Gated in CI by the `assetresolver-swap` job in `.github/workflows/ci.yml` (flag-gated
build + docker `minio/minio:latest` service on port 9000 +
`ctest -R 'x3d_assetresolver(_backend|_swap)'`). On every PR; a future parity break
fails the merge.

## How it is used

The seam is invoked from **two distinct call sites**:

### Consumer-side (contract A — render-time)

A consumer (PoC renderer, Godot embedder, CAVE master, …) holds the `AssetResolver`
and calls it per `TextureRef::Source::Url`. The PoC's `examples/poc_renderer/main.cpp:394`
shows the wiring (`makeLocalFileResolver` — the pre-AssetResolver-seam file-I/O
implementation; the new libcurl / S3 backends drop in as direct replacements).

### SDK-side (contract B — parse-time)

`runtime/parse/X3DParse.hpp:141` `localFileProtoResolver` is the **separate parse-time
PROTO seam** (`ProtoDeclarationResolver`, not `AssetResolver`) — file-only by default,
delegating `http(s)://` and `urn:` to embedder override territory. The PRF-6 card
(http/urn EXTERNPROTO) is the parse-time seam's genericity proof; it unblocks here
because the bytes-fetching seam it sits on top of is now proven.

## What the seam unblocks

The AssetResolver card ships four P1 dependency cards from "blocked" to "workable":

- **NSN-*** — LoadSensor (NSN-1..9): now wireable as an active System observing per-tick
  child URL-object load state.
- **PRF-6** — http/urn EXTERNPROTO: parse-time `ProtoDeclarationResolver` becomes
  writeable for `http://` / `urn:` URLs (its own swap-test is a separate card).
- **CONF-CRITIC-2** — Script external-URL: scripts with `@url` can fetch external bytes.
- **SCR-005** — Script autoRefresh / autoRefreshTimeLimit: periodic re-fetch becomes
  possible.

The AssetResolver card itself does **not** ship these — they become workable now that
the IO seam is proven generic; each flips its own findings when its card ships.

## Files and tests

| File | Role |
|---|---|
| `runtime/extract/AssetResolver.hpp` | The seam type — header-only, std-only, golden-untouched |
| `runtime/io/curl/HttpResolver.{hpp,cpp}` | Backend A — libcurl HTTP, isolated TU |
| `runtime/io/curl/tests/asset_resolver_backend_a_test.cpp` | Per-backend test for A |
| `runtime/io/s3/S3Resolver.{hpp,cpp}` | Backend B — AWS C++ SDK S3, isolated TU |
| `runtime/io/s3/tests/asset_resolver_backend_b_test.cpp` | Per-backend test for B |
| `runtime/io/tests/asset_resolver_swap_test.cpp` | The swap-test (the genericity proof) |
| `runtime/io/tests/fixtures/{f1,f2,f3}.bin` | 100-byte fixture payloads |
| `include/x3d/sdk.hpp:50` | `AssetResolver` re-export with `[STABLE]` marker |
| `docs/wiki/decisions/0023-assetresolver-second-backend-swap-test.md` | The ADR |
| `docs/superpowers/specs/2026-06-23-assetresolver-http-design.md` | The design spec |

| ctest target | What it covers |
|---|---|
| `x3d_assetresolver_backend_a` | Backend A: URL prefix + libcurl error path |
| `x3d_assetresolver_backend_b` | Backend B: URL prefix + `s3://` parse + SDK init |
| `x3d_assetresolver_swap` | Genericity proof: byte-equality A==B over shared fixtures, failure-mode parity |

The `x3d_assetresolver_b8_test` test (predecessor) pins the seam's two-invocation-contract
behavior independently of the new backends.

## Related specs and ADRs

- [ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Swap-Test](../decisions/0023-assetresolver-second-backend-swap-test.md)
  — the genericity proof.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Swap-Test](../decisions/0022-scriptengine-second-backend-swap-test.md)
  — the pattern this row follows.
- [Seam-Status Matrix](../seam-status.md) — the live tracker; AssetResolver/IO is GREEN.
- [Extraction Pipeline](extract.md) — the consumer-side wiring for `TextureRef::Source::Url`.
- [Ext firewall](ext-firewall.md) — `externalGeometryResolver` / `PackedMesh` lives here.
- Spec: `docs/superpowers/specs/2026-06-23-assetresolver-http-design.md`.
