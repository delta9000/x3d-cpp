---
title: "ADR-0052: Movie audio uses the existing PCM Buffer source"
summary: SoundSystem accepts MovieTexture as a Sound source through an injected movie-audio decoder. Decoded mono PCM crosses AudioBackend once as a Buffer; MediaTimeSystem supplies playback state and activation speed.
tags: [adr, sound, movie, audio, seam]
updated: 2026-09-26
related:
  - 0041-moviedecoder-seam-royalty-free-defaults.md
  - 0050-audio-pcm-and-ellipsoid-seam.md
  - ../subsystems/sound.md
---

# ADR-0052: Movie audio uses the existing PCM Buffer source

## Status

Accepted

## Context

Sound.source accepts MovieTexture (§16.4.17). The MovieDecoder callback in
ADR-0041 returns video frames. SoundSystem already turns decoded AudioClip PCM
into an AudioBackend Buffer node under ADR-0050. Adding audio samples to every
video frame would duplicate the audio track and tie audio delivery to the
renderer frame rate.

## Decision

SoundSystem has a separate `setMovieAudioDecoder(AudioDecoder)` injection. It
resolves MovieTexture URLs with `AssetKind::Movie`, passes the bytes to that
decoder, and creates one Buffer node from the returned mono PCM. Pending
resolution retries on later ticks. `load=FALSE` defers the fetch. The decoder
is optional; without one, the source stays silent. No public SDK header
includes a codec or opens media.

The Buffer follows MovieTexture `isActive` and `isPaused`. Its PlaybackRate is
the speed captured by MediaTimeSystem at activation, including the active
`set_speed` rule. The consumer reports the movie's duration with
`reportMovieDuration`; SoundSystem does not infer video duration from the audio
track. The existing AudioBackend interface and both backend Buffer
implementations remain unchanged.

`runtime/io/plmpeg/` supplies `makePlMpegMovieAudioDecoder()` for MPEG program
streams with MP2 audio. It decodes the first audio stream to mono PCM. Raw
elementary video has no audio track and returns an unsuccessful result. A
consumer can inject another decoder for other formats.

## Consequences

The whole decoded track is held in memory once per Sound source, matching the
AudioClip Buffer contract. Video and audio decoders may read the same URL
independently through the consumer's resolver. Long streaming movies would
need a separate streaming audio contract. Tests cover a Sound with synthetic
PCM across activation, pause, resume, and stop, and an MP2 program-stream
fixture through the pl_mpeg adapter.
