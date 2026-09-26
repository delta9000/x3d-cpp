# Sound — conformance

_Generated. Levels 1,2 · 21 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Analyser | 2 | ✓ | — | ✗ | SND-5 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| AudioClip | 1 | ✓ | — | ✓ | MULTI-INHERIT, TDN-5 | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode, X3DUrlObject |
| AudioDestination | 2 | ✓ | — | — | SND-7 | X3DChildNode, X3DSoundDestinationNode, X3DSoundNode |
| BiquadFilter | 2 | ✓ | — | ◑ | SND-1, SND-2, SND-8 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| BufferAudioSource | 2 | ✓ | — | ✗ | SND-4 | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode, X3DUrlObject |
| ChannelMerger | 2 | ✓ | — | — | SND-6 | X3DChildNode, X3DSoundChannelNode, X3DSoundNode |
| ChannelSelector | 2 | ✓ | — | — | SND-6 | X3DChildNode, X3DSoundChannelNode, X3DSoundNode |
| ChannelSplitter | 2 | ✓ | — | — | SND-6 | X3DChildNode, X3DSoundChannelNode, X3DSoundNode |
| Convolver | 2 | ✓ | — | ✗ | SND-5 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| Delay | 2 | ✓ | — | ✗ | SND-5 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| DynamicsCompressor | 2 | ✓ | — | ✗ | SND-5 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| Gain | 2 | ✓ | — | ◑ | SND-1, SND-2, SND-8 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |
| ListenerPointSource | 2 | ✓ | — | ◑ | SND-3, SND-GAIN-TYPE | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode |
| MicrophoneSource | 2 | ✓ | — | ✗ | SND-4 | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode |
| OscillatorSource | 2 | ✓ | — | ◑ | SND-1, SND-2, SND-9 | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode |
| PeriodicWave | 2 | ✓ | — | — | SND-9 | X3DChildNode, X3DSoundNode |
| Sound | 1 | ✓ | — | — | — | X3DChildNode, X3DSoundNode |
| SpatialSound | 2 | ✓ | — | — | SND-3 | X3DChildNode, X3DSoundNode |
| StreamAudioDestination | 2 | ✓ | — | — | SND-7 | X3DChildNode, X3DSoundDestinationNode, X3DSoundNode |
| StreamAudioSource | 2 | ✓ | — | ✗ | SND-4 | X3DChildNode, X3DSoundNode, X3DSoundSourceNode, X3DTimeDependentNode |
| WaveShaper | 2 | ✓ | — | ✗ | SND-5 | X3DChildNode, X3DSoundNode, X3DSoundProcessingNode, X3DTimeDependentNode |

## Findings

- **SND-1** [major/OPEN] — §16.4.12, 16.4.15, 16.4.4: enabled field ignored on every wired sound node — a disabled Oscillator/Gain/BiquadFilter still synthesizes/processes.
  - SoundSystem buildChild/pushParams never read getEnabled(); §16 defaults enabled TRUE but enabled=FALSE shall disable the node. Actionable now (no seam) — gate createNode/setParam (and bypass vs mute per §16 node semantics).
- **SND-2** [major/OPEN] — §8.2.4.1, 16.4.15: X3DTimeDependentNode lifecycle ignored — sound nodes emit unconditionally; startTime/stopTime/pauseTime/resumeTime/loop not honored and isActive/isPaused/elapsedTime never emitted.
  - SoundSystem has no activation clock — the oscillator plays from t=0. Gating a synthesized source on startTime/stopTime is doable now (no media seam); emitting isActive/isPaused as routed events needs an event-cascade hookup. Mirrors TimeSensor (TDN-*); relates to TDN-5 for media sources.
- **SND-3** [major/DEFERRED] — §16.4.16, 16.4.13: Spatialization PARTIAL — SpatialSound/ListenerPointSource equal-power pan + three DistanceModel modes proven via swap-test (ADR-0026); the §16.4.17 Sound ellipsoid now ships (ADR-0050); HRTF (SpatialSound.enableHRTF) and Doppler (dopplerEnabled) still deferred.
  - The classic Sound node's ellipsoid (minFront/maxFront/minBack/maxBack, direction, intensity, spatialize) crosses the seam as geometry via DistanceModel::Ellipsoid and both backends evaluate it (swap-test F4, sound_immersive_test), so Sound is off this finding. SoundSystem wires SpatialSound+ListenerPointSource through a Panner node; equal-power pan (BuiltinDsp) and ma_spatializer (MiniaudioBackend) agree on structural spatial invariants (ear-sign, symmetry, monotonic distance falloff over Linear/Inverse/Exponential). NOT proven: HRTF (no second reference HRTF renderer), Doppler. Those remain deferred to v2. See ADR-0026 scope-honesty section. Replacing the ellipsoid attenuation model with a simpler spherical/OpenAL-aligned model, and adding HRTF, have been recurring, long-standing priorities in X3D audio discussion — not a novel or low-priority ask — worth weighting accordingly when scheduling v2.
- **SND-4** [major/DEFERRED] — §16.4.5, 16.4.20, 16.4.14: Audio source breadth — OscillatorSource and AudioClip are built; BufferAudioSource/StreamAudioSource/MicrophoneSource subtrees are skipped (buildChild returns early).
  - AudioClip ships (ADR-0050): url fetched through the AssetResolver oracle (AssetKind::Audio, Pending retried), decoded by an injected AudioDecoder (reference WAV decoder in runtime/io/wav/), played as a Buffer node holding decoded mono PCM; duration_changed posted; isActive/isPaused (MediaTimeSystem, TDN-5), pitch and gain drive playback. Not read: AudioClip.description. BufferAudioSource can reuse the Buffer node (its buffer field is already PCM); StreamAudioSource and MicrophoneSource need a streaming/capture contract.
- **SND-5** [minor/DEFERRED] — §16.4.1, 16.4.6, 16.4.7, 16.4.9, 16.4.21: Sound-processing breadth — only Gain + BiquadFilter are built; Analyser/Convolver/Delay/DynamicsCompressor/WaveShaper subtrees are skipped.
  - Each is a clean AudioBackend extension point (add a NodeKind + DSP in the backend .cpp). No new seam needed — deferred on demand (no consumer yet).
- **SND-6** [minor/DEFERRED] — §16.4.10, 16.4.11, 16.4.8: Channels + multi-channel PARTIALLY UNBLOCKED — stereo render path (renderStereo) landed (ADR-0026); ChannelMerger/Selector/Splitter still not built; channelCount/channelCountMode/channelInterpretation still ignored.
  - The AudioBackend now has renderStereo() proven via the swap-test (both backends render stereo output for Panner nodes). The stereo path is the seam prerequisite for multi-channel; the ChannelMerger/Selector/Splitter nodes and channel-aware routing still require dedicated NodeKinds and a channel-count-aware buffer contract. Deferred. Multi-channel routing has moved beyond a speculative extension: it was an actively-proposed X3D4 spec feature with competing routing-node proposals, and at least one working prototype implementation has demonstrated SFInt32 routing on both source and destination sides plus connect/disconnect handling — concrete precedent for the buffer-contract shape this seam will eventually need.
- **SND-7** [minor/OPEN] — §16.4.3, 16.4.19: AudioDestination.gain ignored by render and maxChannelCount is read but unused (mono); StreamAudioDestination not built.
  - SoundSystem reads maxChannelCount into params but render() is mono and never applies the destination gain (§16.4.3). The gain pass-through is a small fix (no seam); multi-channel + stream output is deferred (SND-6).
- **SND-8** [minor/DEFERRED] — §16.4.12, 16.4.4: tailTime ignored on processing nodes — a stopped Gain/BiquadFilter does not continue emitting for tailTime seconds.
  - Tied to the missing activation lifecycle (SND-2); the backend renders steady-state with no stop edge to tail from. Resolve alongside SND-2.
- **SND-9** [minor/DEFERRED] — §16.4.15, 16.4.18: periodicWave ignored — OscillatorSource is always Sine; a referenced PeriodicWave (custom waveform via real/imag DFT terms) is not realized.
  - Spec-correct default (periodicWave NULL = sine), so not a bug — but custom waveforms are unsupported. The seam already carries a Waveform enum (Sine/Square/Sawtooth/Triangle); arbitrary PeriodicWave needs a DFT-coefficient param. Deferred.
- **TDN-5** [major/CLOSED] — §8.2.4.1, 16.4.2, 18.4.2: AudioClip/MovieTexture have no time-lifecycle System — startTime/loop/isActive inert.
  - MediaTimeSystem (attached by attachStandardRuntime) runs the shared X3DTimeDependentSystem lifecycle for AudioClip and MovieTexture: enabled, loop, and a cycle of duration_changed / pitch (AudioClip) or / speed (MovieTexture); an unknown duration plays until stopTime (media_time_test). Decoding and playback remain separate (SND-4).
- **MULTI-INHERIT** [minor/CLOSED] — §16.4.2, 18.4.2: MovieTexture declared under two abstract node types; engine handles it via ADR-0004 virtual mixins. AudioClip is the clean single-node pattern (named by association).
  - UOM nominates one primary Inheritance + AdditionalInheritance; bindings emit every base public virtual (MovieTexture.hpp:40-42), the shared X3DNode collapses, and reflection accessors are qualified by declaring ancestor (MovieTexture.cpp:19,89,106). containerField defaults deterministic - MovieTexture=texture, AudioClip=source. No engine impact (see ADR-0004). 4.1 - confirmed; 4.1 did NOT do the X3DSoundSourceObject interface recast (still multiple inheritance), so the virtual-mixin approach remains the durable answer.
- **SND-GAIN-TYPE** [low/CLOSED] — §16.4.13: gain typed SFInt32 in ISO prose vs SFFloat in X3DUOM; engine correct-by-UOM.
  - Upstream prose erratum, not an engine gap. The X3DUOM inherits SFFloat from X3DSoundSourceNode; x3d-cpp is correct (ListenerPointSource.cpp:84-95). Add a regression assert gain.type == SFFloat to pin against codegen drift. 4.1 - confirmed; 4.1 UOM still SFFloat.

