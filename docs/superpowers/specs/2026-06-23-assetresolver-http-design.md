# Prove the AssetResolver/IO seam generic: libcurl HTTP + S3 SDK — Design

**Date:** 2026-06-23
**Status:** Design proposal — pending human approval before the implementation workflow launches.
**Project card:** "Prove AssetResolver/IO generic: curl + object-store" (Seam=AssetResolver/IO, Phase 1).
**Issue:** #12.
**Workflow:** [Card → Done](../../wiki/guides/card-to-done-workflow.md) (substantial card → spec + ADR
first) · [Subagent discipline](../../wiki/guides/workflow-subagent-discipline.md) (five-phase spine).

## Goal

Add **two independent `AssetResolver` backends** behind the existing seam with **zero changes to
the runtime core**, and prove the seam is genuinely generic via a **behavioral swap-test**:
identical fixture bytes served through Backend A vs Backend B must yield **identical observable
output** (`AssetResult.bytes` byte-equal across the two backends).

This is the **Phase-1 proof** for the product thesis (the pattern was established by
[ADR-0022](../wiki/decisions/0022-scriptengine-second-backend-swap-test.md) — ScriptEngine's
QuickJS pilot). AssetResolver/IO is the **highest-leverage** next seam because the only
bytes-fetching backend today is the PoC's `makeLocalFileResolver` (`examples/poc_renderer/main.cpp:394`);
shipping curl + S3 unblocks LoadSensor (NSN-*), http/urn EXTERNPROTO (PRF-6 card),
Script external-URL (CONF-CRITIC-2), and autoRefresh (SCR-005).

## The seam as it stands (grounded)

- `runtime/extract/AssetResolver.hpp` — the seam type:
  ```
  using AssetResolver = std::function<AssetResult(const std::string &url, AssetKind kind)>;
  ```
  where `AssetKind ∈ { Texture, Movie, Inline, ExternProto, ExternalGeometry }` and
  `AssetStatus ∈ { Ready, Pending, Failed }` (Pending legal **only** on the render-time contract-A
  path; the parse-time contract-B path requires Ready/Failed synchronously — see the file header).
- **Already shaped for this.** Header-only POD + `std::function`; no node/transform dependency.
  Embedder-owned lifetime (the SDK never invokes it for the texture path and never does I/O).
- Existing concrete instances:
  - `makeLocalFileResolver` (PoC, `examples/poc_renderer/main.cpp:394`) — reads bytes relative
    to a scene directory; the **only** bytes-fetching backend in tree today.
  - `localFileProtoResolver` (`runtime/parse/X3DParse.hpp:141`) — a different callback type
    (`ProtoDeclarationResolver`) for the **parse-time** PROTO seam; this card does **not** change
    that — PRF-6 unblocks separately.
- `AssetKind::Inline` / `AssetKind::ExternProto` are the contract-B entry points; the new
  backends must accept them and return Ready/Failed synchronously (no Pending) — that's the
  observable-parity assertion the swap-test pins.

## Deliverable 1 — Backend A: libcurl HTTP

`runtime/io/curl/HttpResolver.{hpp,cpp}` : `AssetResolver`. Mirrors the existing
`x3d_quickjs` isolation discipline (`CMakeLists.txt:308-345`).

- **Vendoring / build isolation** — new `option(X3D_CPP_BUILD_CURL "Build libcurl HTTP
  AssetResolver backend (OFF default)" OFF)` → an isolated `x3d_curl` STATIC lib that is the
  single TU where libcurl meets the seam; libcurl linked **PRIVATE** so its flags/headers never
  leak to consumers. Default build (option OFF) is **unchanged** (`find_package(CURL REQUIRED)`
  when ON; no FetchContent — libcurl is a system library universally available).
- **Sync by default.** `HttpResolver::resolve(url, kind)` blocks on `curl_easy_perform` and
  returns `AssetResult::makeReady(bytes)` on `CURLE_OK`, `AssetResult::makeFailed()` otherwise.
- **Optional async (deferred-bytes).** For contract-A render-time callers, an opt-in
  `makeAsyncHttpResolver()` returns Pending on the first call and Ready on a later frame;
  implemented as `std::async` + a thread-local pending-map keyed by URL. The Pending path is the
  feature the future consumer needs; the swap-test pins the synchronous path.
- **URL subset.** Honors `http://` and `https://`; returns Failed for `urn:` (embedder override
  territory, mirroring `localFileProtoResolver`'s exclusion).

## Deliverable 2 — Backend B: S3 SDK + docker minio

`runtime/io/s3/S3Resolver.{hpp,cpp}` : `AssetResolver`. Same isolation discipline.

- **Vendoring / build isolation** — new `option(X3D_CPP_BUILD_S3 "Build S3 SDK AssetResolver
  backend (OFF default)" OFF)` → isolated `x3d_s3` STATIC lib; AWS SDK linked **PRIVATE**.
  Default build (option OFF) is **unchanged**. Heavyweight dep (aws-sdk-cpp core + s3) —
  `find_package(AWSSDK REQUIRED COMPONENTS s3)` when ON (system-installed is the practical
  choice; FetchContent of aws-sdk-cpp is a CI build-time cost the project doesn't need).
- **Sync via `GetObject`.** Maps `(url, kind)` to a bucket + key (URL form
  `s3://bucket/key?...`); calls `s3::GetObject` and returns `AssetResult::makeReady(bytes)` on
  success, `AssetResult::makeFailed()` otherwise.
- **Docker minio fixture.** The swap-test ctest requires a running minio at `$X3D_S3_ENDPOINT`
  (default `http://localhost:9000`); CI starts it via `docker run -d -p 9000:9000 -p 9001:9001
  minio/minio server /data` before the swap-test job and tears it down after. Local dev: same
  command by hand.
- **URL subset.** Honors `s3://` and the `https://*.s3.*.amazonaws.com` virtual-hosted form;
  returns Failed otherwise.

## Deliverable 3 — the swap-test (the genericity proof)

`runtime/io/tests/asset_resolver_swap_test.cpp` — a ctest binary that drives the *same* fixtures
through Backend A (libcurl) and Backend B (S3) and asserts byte-equality of the returned
`AssetResult.bytes`. Mechanism:

1. **Fixture content** — a small set of N known-byte payloads (`fixtures/a.bin` … `fixtures/n.bin`)
   committed to the repo, totalling ≤ 1 MiB. Uploaded to minio at swap-test setup time
   (`mc anonymous set` or a small `aws s3 cp` script in CI).
2. **Local HTTP server** — the test binary starts an in-process HTTP server on a free port (one
   of the well-known single-header C++ libs, linked PRIVATE to the test binary), serving the same
   fixture bytes from disk on `http://localhost:<port>/fixtures/<name>`.
3. **Loop** — for each fixture `f` and each backend `{A, B}`:
   - Backend A → `resolverA("http://localhost:<port>/fixtures/f", AssetKind::Texture)`.
   - Backend B → `resolverB("s3://bucket/fixtures/f", AssetKind::Texture)`.
   - Assert `resultA.ready() && resultB.ready()` and `resultA.bytes == resultB.bytes`.
4. **Failure path** — also assert each backend returns `Failed` for a missing key (Backend A:
   `404`; Backend B: `NoSuchKey`) and that the failure *modes* are observable-equal (both return
   `AssetStatus::Failed`, both have empty `bytes`).

Gated in CI: dedicated `AssetResolver seam swap-test` job in `.github/workflows/ci.yml` (flag-gated
`-DX3D_CPP_BUILD_CURL=ON -DX3D_CPP_BUILD_S3=ON`, docker service for minio, `ctest -R
'x3d_assetresolver(_backend|_swap)'`). Mirrors the `QuickJS seam swap-test` job.

## Definition of Done (the card's checklist)

- [ ] `AssetResolver` interface frozen — promote `[EXPERIMENTAL]` → `[STABLE]` in
      `include/x3d/sdk.hpp:131` (proven by two independent backends implementing it unchanged).
- [ ] Backend A (libcurl HTTP) wired behind `X3D_CPP_BUILD_CURL`, no core `#ifdef`.
- [ ] Backend B (S3 SDK) wired behind `X3D_CPP_BUILD_S3`, no core `#ifdef`.
- [ ] **Swap-test green**: every fixture produces byte-equal `AssetResult.bytes` under A and B
      (and equal `Failed` for missing keys).
- [ ] Findings this seam unblocks closed: NSN-* (LoadSensor), PRF-6 (http/urn EXTERNPROTO —
      parse-time seam; this card does **not** itself implement it but unblocks the work),
      CONF-CRITIC-2 (Script external-URL), SCR-005 (autoRefresh).
- [ ] Docs: **seam-status matrix** row flipped GREEN + new `system-asset-io` subsystem page +
      ADR-0023 + `v1-capabilities.md` claim row.

## Phased implementation plan (five-phase workflow units)

| Unit | Work | Isolation |
|---|---|---|
| **U1 — Backend A** | `HttpResolver` skeleton + CMake (`X3D_CPP_BUILD_CURL` → `x3d_curl`) + sync path + URL-subset rules + a per-backend unit test (`x3d_assetresolver_backend_a`). | New files; CMake edit is sequential. |
| **U2 — Backend B** | `S3Resolver` skeleton + CMake (`X3D_CPP_BUILD_S3` → `x3d_s3`) + sync path + URL-subset rules + a per-backend unit test (`x3d_assetresolver_backend_b`). | New files; CMake edit is sequential. |
| **U3 — Swap-test** | The `x3d_assetresolver_swap` ctest: in-process HTTP server + minio fixture upload script + byte-equality loop + failure-mode parity; CI job wired. | New test files + a workflow edit — sequential. |
| **U4 — Freeze + docs** | Promote interface `[EXPERIMENTAL]→[STABLE]`; flip matrix row GREEN; ADR-0023; new `system-asset-io` subsystem page; `v1-capabilities.md` row; close findings. | Docs + one header — sequential. |

Each unit: TDD (red→green), per-unit adversarial review (`buildPassed`/`testsPassed`/
`goldenUntouched`/`approved`), ≤2 fix rounds. Gate: `mise run ci` + swap-test green + matrix row
flipped + ADR-0023 accepted.

## Golden / codegen rule

**Codegen-free.** No `generated_cpp_bindings/` or template touch — the AssetResolver type is a
header-only `std::function`; this adds runtime files + two opt-in CMake libs. Golden hash MUST
stay byte-identical. Default build (CURL OFF, S3 OFF) is behavior-unchanged.

## Risks

1. **AWS SDK weight** — aws-sdk-cpp is a large dependency; `find_package` keeps it out of the
   default build, but the swap-test job still pays a one-time compile cost. Mitigation:
   `X3D_CPP_BUILD_S3` OFF by default; document the install step in `docs/wiki/build-and-mise.md`.
2. **Docker in CI** — the swap-test job needs a docker service (minio). Mitigation: GH Actions
   `services:` block; document the local-dev `docker run` command.
3. **Pending lifetime (contract-A)** — both backends must honor the Pending→Ready transition for
   the render-time path. Mitigation: ship the sync path in U1/U2; deferred-bytes is a follow-up
   that is not pinned by the swap-test.
4. **Genericity leak** — if S3 forces an `AssetResolver` signature change, that's a *finding*,
   not a failure: the spec's whole point is to surface it. The swap-test cannot pass if the
   interface changes — the divergence is loud, not silent.

## ADR-0023 (to land with U4)

"Second `AssetResolver` backend (S3 SDK) + behavioral swap-test as the genericity proof" — the
**same pattern as ADR-0022** applied to the highest-leverage IO seam. The matrix row flips from
NOT-YET-PROVEN to GREEN; the recipe for the remaining seams (Physics, Audio, FontMetrics,
TextureResolver, GeoProjection, Consumer) is unchanged.

## Related

- [ADR-0022](../wiki/decisions/0022-scriptengine-second-backend-swap-test.md) — the pattern this
  card follows.
- [Seam-Status Matrix](../wiki/seam-status.md) — the live tracker; AssetResolver/IO is
  NOT-YET-PROVEN today.
- `docs/superpowers/specs/2026-06-20-physics-jolt-seam-design.md` — the flag-gated
  isolated-backend precedent this card's CMake pattern mirrors.
