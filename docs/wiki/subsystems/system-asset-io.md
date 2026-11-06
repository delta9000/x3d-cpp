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

# Asset resolution

`AssetResolver` maps `(url, AssetKind)` to `AssetResult`: bytes and one of
`Ready`, `Pending`, or `Failed`. Its definition is in
`runtime/extract/AssetResolver.hpp`. The host owns loading policy.

## Invocation contracts

Render-time callers can receive `Pending` and retry later. Synchronous
parse-time callers require an immediate result: `Failed` tries the next URL,
while `Pending` aborts the recursive load. Do not turn a pending result into
permission to use another resolver.

`runtime/parse/AssetDocumentResolvers.hpp` composes paired Inline and
EXTERNPROTO loaders. The pair preserves resolver policy and fetched-source
bases through nested loading and declaration caching. See
[recursive asset loader policy](parse-readers.md#recursive-asset-loader-policy).

## Scheme routing

`makeSchemeRouter` in `runtime/extract/SchemeRouter.hpp` dispatches by URL
scheme. Unknown schemes use the configured fallback or fail. Register the
schemes your application permits; routing does not add transport support.

## File resolver

`runtime/io/file/FileResolver.hpp` confines reads to a configured root. It
canonicalizes paths and checks containment, including symlink resolution.
The default parse helpers also apply local-path confinement.

## HTTP and FTP

Enable `X3D_CPP_BUILD_CURL` to build `x3d_curl`.
`runtime/io/curl/HttpResolver.hpp` exposes `makeHttpResolver()` and
`makeFtpResolver`. Both perform synchronous requests through libcurl.

HTTP permits HTTP/HTTPS only, including redirects; FTP has a separate protocol
allowlist. The adapters apply byte and time bounds, a redirect cap, and
post-DNS address checks. With `allowPrivateNetworks=false`, environment proxies
are disabled and connections to classified private or reserved addresses fail.
Opting into private networks also permits environment proxies.

## Consumers and evidence

The CPU reference host composes file, HTTP, HTTPS, and FTP routes with the
stb byte decoder. `runtime/io/tests/asset_resolver_swap_test.cpp` checks HTTP
fixture bytes and missing assets. Despite its historical name, it is an HTTP
contract test. `runtime/io/tests/file_resolver_test.cpp` checks confined files;
`runtime/io/curl/tests/` checks HTTP address and transport policy.

For conformance status, see the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml).
The [decision record](../decisions/0023-assetresolver-second-backend-swap-test.md)
contains the transport decision and its amendment.
