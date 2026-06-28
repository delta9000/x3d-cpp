---
title: "ADR-0041: MovieDecoder Seam — Royalty-Free Codecs Ship, Encumbered Codecs Plug In"
summary: MovieTexture frame decode is a consumer-side seam, exactly like TextureResolver/FontMetrics — the core never links a video codec. The SDK ships permissively-licensed AND patent-clean backends by default (pl_mpeg for MPEG-1, with Theora and WebM/VP8-9 planned) and leaves patent-encumbered / copyleft codecs (H.264/H.265, FFmpeg, GStreamer, OS-native) to implementors behind the same frozen seam. The dividing line is "royalty-free + permissive ships; patented and/or copyleft plugs in." Unlike the font/texture seams, video codecs partition by container/codec, so the genericity proof is a per-backend semantics-contract test, not bit-identical-output swap.
tags: [adr, seam, movietexture, video, codec, licensing, royalty-free, pl-mpeg, theora, webm, proposed]
updated: 2026-06-27
related:
  - ../architecture.md
  - ../seam-status.md
  - 0024-textureresolver-second-backend-swap-test.md
  - 0025-fontmetrics-second-backend-swap-test.md
  - 0026-audiobackend-second-backend-swap-test.md
  - ../subsystems/system-texture-decode.md
---

# ADR-0041: MovieDecoder Seam — Royalty-Free Codecs Ship, Encumbered Codecs Plug In

## Status

Proposed (2026-06-27). No code yet — this ADR fixes the seam shape and the
ship-vs-plug-in licensing line so they get reviewed before implementation. The
intended first slice is Backend A (pl_mpeg, MPEG-1) wired into the PoC renderer,
which also covers the entire Web3D/NIST conformance MovieTexture corpus (all
`.mpg`). Theora and WebM follow as additional backends behind the frozen seam.

## Context

The product thesis (see [ADR-0022](0022-scriptengine-second-backend-swap-test.md))
is that x3d-cpp is **unopinionated and pluggable**: every place a renderer/engine
plugs in is an abstract **seam**, and the runtime core stays spec-correct and
backend-free. The core **never decodes bytes** — `TextureResolver`
([ADR-0024](0024-textureresolver-second-backend-swap-test.md)) surfaces image URLs
and the consumer decodes; `FontMetrics`
([ADR-0025](0025-fontmetrics-second-backend-swap-test.md)) surfaces advances and
the consumer rasterizes; `AudioBackend`
([ADR-0026](0026-audiobackend-second-backend-swap-test.md)) takes synthesized PCM
from a consumer-owned engine.

MovieTexture is already half-modeled on this principle. The extractor surfaces it
as `TextureRef::Source::Movie` (`runtime/extract/RenderItem.hpp` — *"descriptor-only,
not exercised by PoC"*), and `X3DTimeDependentSystem`
(`runtime/events/X3DTimeDependentSystem.hpp`) already drives the time-dependent
fields (`startTime`/`stopTime`/`loop`/`speed` → `isActive`/`elapsedTime`). What is
missing is the **frame decode path**: a way for the renderer to obtain the RGB
frame for the movie's current media time and upload it as a texture. Today the PoC
skips `Source::Movie` entirely, so MovieTexture scenes render with the white/last
fallback (visible in the conformance visual sweep as blank/grey quads).

The hard part is **not** the plumbing — it is the **licensing and patent surface**
of video codecs, which is far worse than images or fonts:

- **FFmpeg / libav** — LGPL-2.1 at the core, but the commonly-built configurations
  (GPL components, `--enable-gpl`) are GPL-3, and either way it bundles decoders
  for patent-encumbered formats.
- **GStreamer** — LGPL framework but a plugin-license soup (base/good/bad/ugly,
  each with different license and patent posture).
- **H.264 / H.265** — active patent pools (MPEG-LA / Access Advance) with
  per-unit royalties regardless of the decoder's software license.

Shipping any of those by default would break the SDK's standing invariant that the
**default build stays MIT / public-domain-clean with no patent exposure**
(the invariant the wuffs and miniaudio backends are flag-gated OFF to preserve).

## Decision

**1. Introduce a `MovieDecoder` seam** — a consumer-supplied callback, the same
shape and threading contract as `TextureResolver` / `FontMetrics` (copyable
`std::function` over std + core types; the SDK never opens a media file or links a
codec). Shape (illustrative, to be frozen in `include/x3d/sdk.hpp` when accepted):

```cpp
// runtime/extract/MovieDecoder.hpp  (header-only, std-only, leaf)
struct VideoFrame {
  std::uint32_t width = 0, height = 0;
  std::vector<std::uint8_t> rgba;   // tightly packed RGBA8, bottom-left origin (GL).
};
enum class FrameStatus { Ready, Pending, Failed };
struct FrameResult { FrameStatus status; VideoFrame frame; /* + makeReady/... */ };

// Called once per active MovieTexture per frame with the media time the
// X3DTimeDependentSystem already computed. The backend owns a per-URL decoder
// context internally (sequential decode is cheap; seeking is the expensive path).
using MovieDecoder =
    std::function<FrameResult(const std::string& url, double mediaTimeSeconds)>;
```

`Pending` semantics mirror the other seams: not-yet-decoded → the consumer keeps
the previously uploaded frame; the render loop **never blocks**. `Failed` → the
white/last fallback. EOF with `loop=FALSE` holds the last frame; `loop=TRUE` wraps
(the time the SDK passes already reflects loop wrap, so the backend just decodes
the requested time).

**2. Draw the ship-vs-plug-in line at "royalty-free AND permissively licensed."**
The SDK ships, as flag-gated isolated `runtime/io/<backend>/` targets (the
`x3d_stbtt` / `x3d_wuffs` pattern — PRIVATE deps, default OFF, decoder-free public
header):

| Tier | Codec | Library | License | Patents |
|---|---|---|---|---|
| **Ship (default-able)** | MPEG-1 | pl_mpeg (single-header) | MIT | expired |
| **Ship (planned)** | Theora | theoraplay / libtheora+libogg | BSD | royalty-free (Xiph) |
| **Ship (planned)** | VP8/VP9 (WebM) | libvpx + libwebm/nestegg demux | BSD / ISC | royalty-free (Google) |
| **Plug in** | H.264 / H.265 | OpenH264 / FFmpeg / GStreamer / OS-native | BSD-binary / LGPL / GPL | **active pools** |

Implementors who need encumbered formats implement the *same* `MovieDecoder` seam
with FFmpeg, GStreamer, or an OS-native decoder (VideoToolbox, Media Foundation) —
that code lives downstream and the SDK core never links it. The line is principled,
not arbitrary: every "ship" codec is *both* permissively licensed *and* free of
active patent royalties, so the default build keeps its MIT/PD-clean, patent-clean
invariant.

**3. Sequence pl_mpeg first.** MPEG-1 (a) covers the entire NIST conformance
MovieTexture corpus and (b) proves the seam end-to-end with one backend before the
heavier Theora/WebM backends are built. pl_mpeg is a single header (demux + decode
in one), so Backend A is the cheapest possible first proof.

## Consequences

- **Core stays clean.** No video codec is ever a dependency of `x3d_cpp`; the
  default build's MIT/PD-clean + patent-clean invariant is preserved. Encumbered
  decode is a downstream licensing decision, made by the party that can make it.
- **Reuses existing machinery.** `Source::Movie` and `X3DTimeDependentSystem`
  already exist; this ADR adds only the frame-decode seam + per-backend targets +
  the PoC's per-frame upload. The PoC's static `--screenshot` path decodes the
  frame at the bound `currentTime` (or frame 0); the realtime loop decodes on tick.
- **WebM is the heavy "ship" backend.** Unlike pl_mpeg (single header) and
  theoraplay (bundles ogg+theora), WebM is libvpx **plus** a Matroska/WebM demuxer
  (libwebm or nestegg) plus glue. It is still permissive + royalty-free, but it is
  the most assembly-required of the three and should land last. AV1 (dav1d, BSD-2,
  also royalty-free) could join the ship tier later for modern content, but no
  conformance scene needs it.
- **Audio is a different seam.** A MovieTexture can also drive a `Sound` node. The
  texture path ignores the audio track; MovieTexture-as-sound routes decoded PCM to
  the existing `AudioBackend` (miniaudio) seam, not the `MovieDecoder`.
- **The genericity proof differs from ADR-0022's swap-test.** The font/texture
  proofs run the *same* fixture (one TTF, one PNG) through *two* backends and assert
  bit-identical output. Video codecs **partition by container/codec** — no two of
  pl_mpeg / theora / vpx can decode the same input file — so there is no
  same-input/same-output swap to assert. The MovieDecoder genericity proof is
  therefore a **per-backend semantics-contract test**: each backend, given its own
  reference clip, must agree on the observable *seam* behavior — `Pending` before
  first frame, EOF→last-frame hold, `loop` wrap, seek-to-time monotonicity, frame
  dimensions — with pixel output checked against that backend's own golden, not
  cross-codec. This is a deliberate, documented deviation: the seam is generic
  (one interface, many format-backends compose) even though it is not
  bit-exact-swappable the way the font/texture seams are. The seam-status row stays
  NOT-YET-PROVEN until ≥2 backends pass the contract test under a CI gate.
- **NOTICE / gating.** Each shipped backend is added to `NOTICE` as a flag-gated
  bundled dependency (the wuffs/miniaudio pattern) and stays OFF in the default
  preset. pl_mpeg → `-DX3D_CPP_BUILD_PLMPEG=ON` (consumed by the PoC like
  `x3d_stbtt`).

## Related

- [ADR-0024: TextureResolver Second Backend Swap-Test](0024-textureresolver-second-backend-swap-test.md) — the decode-is-a-seam precedent (stb_image + wuffs).
- [ADR-0025: FontMetrics Second Backend Swap-Test](0025-fontmetrics-second-backend-swap-test.md) — the consumer-rasterizes precedent and the bit-identical swap-test this ADR deliberately departs from.
- [ADR-0026: AudioBackend Second Backend Swap-Test](0026-audiobackend-second-backend-swap-test.md) — where a MovieTexture's audio track routes.
- [Seam-Status Matrix](../seam-status.md) — the live genericity tracker (MovieDecoder row: NOT-YET-PROVEN).
