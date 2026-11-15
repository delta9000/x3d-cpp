---
title: Seams, ports and backends
summary: Why resource IO and engines sit behind host-supplied interfaces, and what a swap-test proves.
tags: [explanation, seams, embedding]
updated: 2026-11-15
related:
  - architecture.md
  - ../reference/seam-matrix.md
  - ../how-to/supply-resources.md
---

# Seams, ports and backends

The simulation and extraction core does no resource IO of its own. It doesn't
decode an image, rasterize a glyph, run a script engine or open a socket.
Each of those is a *seam*: an interface the core defines and the host fills in.

## The idea

A seam is a port in the ports-and-adapters sense, also called a service
provider interface. The core owns the interface. The host supplies the adapter,
which this project calls a *backend*. Most seams are a `std::function` value;
the script seam is an abstract class.

The split lets the SDK fit a host's existing file access, caching, image
formats, fonts and threading. Each interface gives the host control over one
of those choices while keeping its dependencies outside the core. The same
runtime can then serve a game engine, a CAVE or a command-line converter.

## What "proven" means

An interface designed around a single implementation tends to leak that
implementation's assumptions. This project treats a seam as backend-agnostic
only when a second, independent backend exists and a swap-test runs the same
contract checks against both. The swap-test decisions (for example
[ADR-0022](decisions/0022-scriptengine-second-backend-swap-test.md) for script
engines and
[ADR-0024](decisions/0024-textureresolver-second-backend-swap-test.md) for
texture decoding) record what each second backend exposed.

Not every seam has reached that point, so stability is stated per seam. The
[seam matrix](../reference/seam-matrix.md) lists each one with its
implementations and tests.

## What ships and what doesn't

The core defaults provide fixed-pitch font metrics and a built-in ellipsoid
geospatial projection. Texture and LoadSensor byte requests fail by default,
so a scene with images uses texture fallbacks until the host supplies a
decoder. Building a backend makes it available to the application; the host
still has to wire it into the appropriate interface.

Additional backends (stb_truetype font metrics, stb_image and Wuffs image
decoding, libcurl fetching, the script engines, physics) live in the source
tree behind the `X3D_CPP_BUILD_*` options in `cmake/x3d/backends.cmake`. Each
has its own header and target and is not re-exported by `x3d/sdk.hpp`.

## One place where the core does read files

Parsing is the exception worth knowing. `parseFile` and the default Inline and
EXTERNPROTO resolvers read local files, because a document that includes other
documents can't be loaded without them. A host that must control every read
passes its own resolvers; see
[Supply resources through the seams](../how-to/supply-resources.md).

## Related

- [Architecture](architecture.md)
