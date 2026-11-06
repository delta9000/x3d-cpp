---
title: Asset Resolver / IO Seam
summary: AssetResolver carries URL bytes and status through a host callback, with local-file and libcurl HTTP/FTP implementations.
tags: [subsystem, seam, asset-resolver, io, libcurl, file, ftp]
updated: 2026-10-02
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
it. The interface remains stable. Local-file and libcurl HTTP/FTP implementations
remain; the unused S3 implementation was removed. See the
[decision amendment](../decisions/0023-assetresolver-second-backend-swap-test.md#amendment-2026-11-05).

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

A single embedder resolver can branch on `AssetKind` to honor both contracts; the reference
backends (`HttpResolver`, `FtpResolver`, `FileResolver`) return `Failed` (never `Pending`) in v1 —
deferred-bytes Pending is a documented follow-up.

## Routing — `makeSchemeRouter` (route by scheme, never by `Failed`)

Every backend self-identifies by URL scheme (HttpResolver → `http(s)://`,
FtpResolver → `ftp(s)://`, a local-file resolver → scheme-less/relative) and returns `Failed` for
everything else, so without a router "wrong backend" and "right backend, genuinely
failed" are indistinguishable. `runtime/extract/SchemeRouter.hpp` (header-only,
std-only, IO-free — a lexical prefix check, no socket/file) mirrors
[`makeMultiFormatTextureResolver`](system-texture-decode.md) by dispatching **before**
calling a backend:

```cpp
// keyed by scheme ("http", "https", "s3", "file", "urn", …; a trailing ':'/'://'
// is tolerated, matching is case-insensitive per RFC 3986). `fallback` handles
// scheme-less / relative urls and is optional.
x3d::runtime::extract::AssetResolver makeSchemeRouter(
    std::map<std::string, x3d::runtime::extract::AssetResolver> backends,
    x3d::runtime::extract::AssetResolver fallback = nullptr);
```

A url whose scheme matches a registered backend is handed to that backend verbatim;
an **unknown scheme yields `Failed` without calling anything** (the router never
guesses, and the `fallback` is not consulted for a named-but-unregistered scheme). A
scheme-less / relative url goes to `fallback` when supplied, else `Failed`. A Windows drive path (`C:\models\a.x3d`) is also treated as scheme-less — its one-letter "scheme" is a drive designator, so it takes `fallback` rather than routing to a non-existent backend. Additive
only — the `[STABLE]` seam type is unchanged.

## FTP and the CPU reference host

The optional `x3d_curl` backend exposes `makeFtpResolver(HttpResolverOptions)` alongside `makeHttpResolver`. FTP has its own protocol allowlist; HTTP redirects cannot switch to FTP. Both adapters enforce byte/time limits and post-DNS private-address restrictions. The FTP tests run a local passive server with an explicit private-network opt-in; the executable retains the default public-network policy.

With `X3D_CPP_BUILD_CPURASTER`, `X3D_CPP_BUILD_STB` and `X3D_CPP_BUILD_CURL` enabled, the CPU host registers file/http/https/ftp routes and sends fetched image bytes to the stb decoder. Local reads are confined to the scene directory. `mise run interchange` builds this configuration and runs its tests. FTP is required by Interchange Annex B.6 as well as Full; a disconnected core SDK does not independently satisfy the host protocol requirement.

## Backends

The file backend is header-only. libcurl implements HTTP/FTP with its library
headers and link dependencies private to `x3d_curl`.

### Backend A — libcurl HTTP (`x3d_curl`, `X3D_CPP_BUILD_CURL`)

`runtime/io/curl/HttpResolver.{hpp,cpp}`:

- **Transport**: synchronous `curl_easy_perform` per call.
- **URL subset**: `http://` and `https://` only; returns `Failed` for `urn:`, `file:`,
  missing scheme.
- **Status mapping**: `CURLE_OK` + 2xx → `Ready(bytes)`; `CURLE_*` or non-2xx → `Failed`.
- **SEC-6 hardening (on by default).** A url like `http://169.254.169.254/` cannot be
  turned into an SSRF against internal services:
  - **Protocol-restricted redirects**: `CURLOPT_PROTOCOLS_STR` /
    `CURLOPT_REDIR_PROTOCOLS_STR = "http,https"` (bitmask `CURLOPT_PROTOCOLS` /
    `CURLOPT_REDIR_PROTOCOLS` on libcurl < 7.85). A redirect to `file:`, `ftp:`, etc. is
    refused.
  - **Redirect cap**: `CURLOPT_MAXREDIRS = 5`.
  - **Response-size cap**: the write callback aborts a body past `maxBytes`
    (default 256 MiB) → `Failed`; `CURLOPT_MAXFILESIZE_LARGE` is set as a hint.
  - **Resolved-address guard**: `CURLOPT_OPENSOCKETFUNCTION` inspects the *resolved*
    sockaddr (post-DNS, per connection and per redirect hop) and refuses loopback
    (127/8, ::1), link-local (169.254/16, fe80::/10), RFC1918 (10/8, 172.16/12,
    192.168/16), CGNAT (100.64/10), unique-local (fc00::/7), unspecified
    (0/8, ::), IPv4 multicast (224/4), reserved/broadcast (240/4), protocol
    assignments (192.0.0/24), and benchmarking (198.18/15) addresses. IPv4
    addresses embedded in IPv6 are classified too —
    IPv4-mapped (`::ffff:a.b.c.d`), IPv4-compatible (`::a.b.c.d`) and NAT64
    (`64:ff9b::/96`) forms all run the IPv4 classifier on the embedded address. A
    hostname or redirect cannot smuggle a private address past it.
  - **No environment proxy**: when `allowPrivateNetworks` is false, `CURLOPT_PROXY = ""`
    disables the `http_proxy`/`https_proxy`/`all_proxy` environment proxies. Without
    this, libcurl would connect to the *proxy*, so the resolved-address guard would
    inspect the proxy rather than the target and a private target could still be
    reached through it. Opting in (`allowPrivateNetworks = true`) re-enables them.
- **Configuration**: `makeHttpResolver(HttpResolverOptions{})` — defaults are the
  hardened ones above; `allowPrivateNetworks = true` widens for local dev/tests
  (the HTTP contract test opts in to reach its loopback fixture server). The no-argument
  `makeHttpResolver()` is source-compatible and uses the defaults.
- **Build option**: `-DX3D_CPP_BUILD_CURL=ON` (OFF default, `find_package(CURL REQUIRED)`).
  Default build (option OFF) is byte-identical and behavior-unchanged.
- **Per-backend test**: `x3d_assetresolver_backend_a` (URL prefix check + libcurl
  error path against unreachable host + the SEC-6 loopback-block / size-cap /
  non-http-redirect cases against an in-process loopback server). The HTTP contract test covers successful reads.

`x3d_http_address_guard` checks the added IPv4 ranges at both endpoints, their
adjacent addresses, and all three supported IPv6 embedding forms without
opening a socket.

## HTTP contract cases — `x3d_assetresolver_swap`

`runtime/io/tests/asset_resolver_swap_test.cpp` starts an in-process loopback
HTTP server. It checks that three fixtures return Ready and match the source
bytes, and that a missing asset returns Failed with empty bytes. Enable
`X3D_CPP_BUILD_CURL` to build it. No service container or credentials are needed.
The target keeps its existing name after removal of the S3 comparison.

## How it is used

The seam is invoked from **two distinct call sites**:

### Consumer-side (contract A — render-time)

A consumer (PoC renderer, Godot embedder, CAVE master, …) holds the `AssetResolver`
and calls it per `TextureRef::Source::Url`. The PoC's `examples/poc_renderer/main.cpp:394`
shows the wiring (`makeLocalFileResolver` — the pre-AssetResolver-seam file-I/O
implementation; the libcurl backends drop in as direct replacements).

### SDK-side (contract B — parse-time)

`runtime/parse/X3DParse.hpp:141` `localFileProtoResolver` is the **separate parse-time
PROTO seam** (`ProtoDeclarationResolver`, not `AssetResolver`) — file-only by default,
delegating `http(s)://` and `urn:` to embedder override territory. The PRF-6 card
(http/urn EXTERNPROTO) is the parse-time seam's genericity proof; it unblocks here
because the bytes-fetching seam it sits on top of is now proven.

`runtime/parse/AssetProtoResolver.hpp` bridges the two: `protoResolverFrom(AssetResolver,
Encoding hint = Unknown)` returns a `ProtoDeclarationResolver` that, for each url in an
EXTERNPROTO's url list (in declared order, `#ProtoName` fragments honored), fetches the
document bytes through the AssetResolver, sniffs the encoding, parses with `parseDocument`,
and returns the matching `ProtoDeclare`. Per contract (B) `Failed` skips to the next
candidate while `Pending` is a hard error (returns null); it never throws. `urn:` is
handed to the resolver like any other url, so it resolves only when the injected resolver
(typically a `makeSchemeRouter` with a `"urn"` entry) owns that scheme — otherwise
`Failed`/skip, matching the file-local default. An embedder that wires
`makeSchemeRouter({{"http", makeHttpResolver()}, {"https", makeHttpResolver()}})` into
`protoResolverFrom` gets http(s):// EXTERNPROTO with no new I/O in the SDK. See the
[PROTO/EXTERNPROTO expansion](proto-expand.md) page for the parse-side detail.

## What the seam unblocks

The AssetResolver card ships four P1 dependency cards from "blocked" to "workable":

- **NSN-*** — LoadSensor (NSN-1..9): now wireable as an active System observing per-tick
  child URL-object load state.
- **PRF-6** — http/urn EXTERNPROTO: parse-time `ProtoDeclarationResolver` becomes
  writeable for `http://` / `urn:` URLs (its own swap-test is a separate card).
- **CONF-CRITIC-2** — Script external-URL: scripts with `@url` can fetch external bytes.
- **SCR-005** — Script autoRefresh / autoRefreshTimeLimit: periodic re-fetch becomes
  possible.

The AssetResolver card itself does **not** ship these — they become workable through
the IO seam; each flips its own findings when its card ships.

## Files and tests

| File | Role |
|---|---|
| `runtime/extract/AssetResolver.hpp` | The seam type — header-only, std-only, golden-untouched |
| `runtime/extract/SchemeRouter.hpp` | `makeSchemeRouter` — per-scheme dispatch, std-only/IO-free |
| `runtime/parse/AssetProtoResolver.hpp` | `protoResolverFrom` — EXTERNPROTO over the AssetResolver seam |
| `runtime/io/curl/HttpResolver.{hpp,cpp}` | Backend A — libcurl HTTP, isolated TU |
| `runtime/io/curl/tests/asset_resolver_backend_a_test.cpp` | Per-backend test for A |
| `runtime/io/tests/asset_resolver_swap_test.cpp` | HTTP fixtures and missing-asset contract |
| `runtime/io/tests/fixtures/{f1,f2,f3}.bin` | 100-byte fixture payloads |
| `include/x3d/sdk.hpp:50` | `AssetResolver` re-export with `[STABLE]` marker |
| `docs/wiki/decisions/0023-assetresolver-second-backend-swap-test.md` | The ADR |
| `docs/superpowers/specs/2026-06-23-assetresolver-http-design.md` | The design spec |

| ctest target | What it covers |
|---|---|
| `x3d_assetresolver_backend_a` | Backend A: URL prefix + libcurl error path |
| `x3d_assetresolver_swap` | HTTP fixture bytes and missing-asset results |
| `x3d_extract_tests` (`scheme_router_test`) | `makeSchemeRouter`: dispatch table, case-insensitive scheme match, unknown-scheme/`fallback` behavior (fake resolvers) |
| `x3d_parse_tests` (`asset_proto_resolver_test`) | `protoResolverFrom`: fetch+parse+fragment selection over a fake in-memory backend, Pending/Failed contract, router+urn |

The `x3d_assetresolver_b8_test` test (predecessor) pins the seam's two-invocation-contract
behavior independently of the new backends.

## Related specs and ADRs

- [ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Swap-Test](../decisions/0023-assetresolver-second-backend-swap-test.md)
  — historical parity decision and removal amendment.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Swap-Test](../decisions/0022-scriptengine-second-backend-swap-test.md)
  — the pattern this row follows.
- [Seam-Status Matrix](../seam-status.md) — the live tracker; AssetResolver/IO is GREEN.
- [Extraction Pipeline](extract.md) — the consumer-side wiring for `TextureRef::Source::Url`.
- [Ext firewall](ext-firewall.md) — `externalGeometryResolver` / `PackedMesh` lives here.
- Spec: `docs/superpowers/specs/2026-06-23-assetresolver-http-design.md`.
