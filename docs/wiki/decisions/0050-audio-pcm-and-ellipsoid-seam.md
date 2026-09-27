---
title: "ADR-0050: Decoded PCM and the Sound Ellipsoid Cross the AudioBackend Seam"
summary: The [STABLE] AudioBackend seam gains a Buffer node kind that receives decoded mono PCM once at createNode, and an Ellipsoid distance model whose four ellipsoid lengths, direction and intensity cross as geometry. Byte fetch stays with the AssetResolver oracle and decoding with a new AudioDecoder function seam (reference WAV decoder in runtime/io/wav). Both backends implement both additions and the swap-test checks that their numbers agree. This brings the classic Sound + AudioClip pair (Immersive profile) into SoundSystem.
tags: [adr, sound, seam, audio, immersive]
updated: 2026-09-26
related:
  - ../subsystems/sound.md
  - 0020-sound-seam.md
  - 0026-audiobackend-second-backend-swap-test.md
  - 0046-loadsensor-assetresolver-oracle.md
---

# ADR-0050: Decoded PCM and the Sound Ellipsoid Cross the AudioBackend Seam

## Status

Accepted

## Context

The Immersive profile needs the classic `Sound` node (§16.4.17) and its
`AudioClip` source (§16.4.2). Before this change `SoundSystem` built only the
v4 Web-Audio-style graph (`OscillatorSource`, `Gain`, `BiquadFilter`,
`AudioDestination`, `SpatialSound`) and skipped `AudioClip` subtrees
(SND-4). The `AudioBackend` seam is frozen `[STABLE]` (ADR-0026), so adding
the two nodes meant deciding what may cross it.

The seam already has a purity rule: positions cross, gains don't. A backend
computes pan and attenuation from geometry, so a second backend with a
different distance law stays swappable. Two things in `Sound`/`AudioClip`
don't fit the existing `NodeParams`:

- **Audio data.** An `AudioClip` names a file. Something has to fetch the
  bytes, decode them, and get samples to the DSP engine.
- **Ellipsoid attenuation.** `Sound` attenuates with two focus-anchored
  ellipsoids (`minFront`/`minBack`, `maxFront`/`maxBack`) along `direction`,
  scaled by `intensity`. None of the three existing `DistanceModel`s
  expresses this.

For audio data there were three options:

1. Let the backend fetch and decode (pass the URL across).
2. Stream decoded blocks across the seam per render call.
3. Decode in the runtime and hand the whole decoded clip to the backend once.

Option 1 puts IO inside every backend and bypasses the `AssetResolver` oracle
that textures, Inline and `LoadSensor` share (ADR-0046). Option 2 couples the
seam to the render cadence and makes every backend re-implement
buffering. Option 3 keeps IO in one place and makes a clip an ordinary
immutable node parameter.

## Decision

**Decoded PCM crosses once.** `NodeKind::Buffer` is a source node whose
`NodeParams` carry `samples` (mono `float`, [-1, 1]) and `sampleRate`,
supplied at `createNode` and never re-sent. Playback is driven by three
per-tick params: `PlaybackState` (0 stopped and rewound, 1 playing, 2 paused),
`PlaybackRate` (`AudioClip.pitch`) and `Gain` (`AudioClip.gain`). The backend
owns the read cursor and the resampling to its output rate.

**Fetch and decode stay in the runtime, behind oracles.** `SoundSystem`
fetches `AudioClip.url` entries in order through the injected
`extract::AssetResolver` (`AssetKind::Audio`; `Pending` retries next tick)
and decodes through an injected `AudioDecoder`
(`runtime/sound/AudioDecoder.hpp`, a `std::function` from bytes to
`DecodedAudio`). The public default is a null decoder, so the SDK stays
IO-free. A reference WAV decoder (PCM 8/16/24/32-bit, float32, extensible;
downmixed to mono) lives in `runtime/io/wav/` for applications to inject.
On a successful decode `SoundSystem` posts `duration_changed`, which feeds
the time lifecycle (`MediaTimeSystem`, TDN-5).

**The ellipsoid crosses as geometry.** `DistanceModel::Ellipsoid` joins the
enum. `NodeParams` gains `direction`, `minFront`, `minBack`, `maxFront`,
`maxBack`, `intensity` and `spatialize`. `SoundSystem` sends only geometry:
the `Sound`'s world location and direction, the lengths scaled by the world
transform, and the viewer pose as the listener (per-tick `Direction*`,
`ListenerPosition*`, `ListenerForward*`, `ListenerUp*`, `Intensity`). Each
backend evaluates §16.4.17 itself: the ellipsoid's reach towards the listener
is `2FB / ((F+B) − (F−B)·cosθ)`; the level is full inside the inner
ellipsoid, falls linearly in dB to −20 dB at the outer one, and is silent
beyond it. `spatialize FALSE` plays centred.

**Both backends implement both additions.** `BuiltinDspBackend` renders the
Buffer inline. `MiniaudioBackend` wraps it in a custom `ma_data_source`, and
for the ellipsoid runs `ma_spatializer` with attenuation off and applies the
ellipsoid gain itself. The swap-test (F4/F5, shared fixtures in
`runtime/sound/tests/immersive_fixtures.hpp`) checks every zone of the
ellipsoid, intensity, centring, and Buffer play/pause/stop/pitch/gain on each
backend. It also checks that the two backends agree on the between-ellipsoids
ratio and on the Buffer RMS.

## Consequences

- The seam contract is amended in place, not versioned: all additions are
  appended enum values and defaulted fields. Existing backends that switch
  over `NodeKind` must add the `Buffer` case (the compiler flags it under
  `-Wswitch`).
- A clip is held in memory, decoded, for its node's lifetime. Long streams
  (`StreamAudioSource`, `MicrophoneSource`) need a different contract and
  remain deferred (SND-4).
- Only WAV has a reference decoder. Other formats are an application's
  `AudioDecoder`, with no seam change.
- The listener for `Sound` is the bound viewpoint's pose, per spec. HRTF and
  Doppler remain deferred (SND-3). `Sound.priority` and `AudioClip.description`
  are read by nothing.
