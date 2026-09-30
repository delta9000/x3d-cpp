// SoundSystem.hpp
// The System that reads the X3D §16 Sound audio graph and drives an (abstract)
// AudioBackend. It walks each AudioDestination, recurses its `children` graph
// bottom-up, creates one backend node per §16 node (OscillatorSource /
// BiquadFilter / Gain / AudioDestination), connects each child -> its parent
// (matching §16 `children` = inputs feeding INTO the parent), and remembers the
// §16-node <-> NodeHandle map. update(now) re-reads each mapped node's (possibly
// route-animated) scalar fields and pushes them via setParam — param animation
// rides the existing event cascade inbound (an author routes an interpolator
// into Gain.gain; the cascade writes the field; SoundSystem reads it each tick),
// exactly like PhysicsSystem reads node state. No new event mechanism.
//
// ENGINE-AGNOSTIC: this layer names NO DSP-engine type and contains NO DSP. It
// depends only on the abstract AudioBackend seam (constructed with a shared_ptr;
// inert when none, exactly like PhysicsSystem/ScriptSystem without a backend) and
// the generated §16 nodes. A seam-purity check greps this header for DSP leakage
// (oscillator math / biquad / coeff / phase / DFT / Goertzel / filter state):
// there is none.
//
// §16 MODEL (grounded in the ISO prose): the audio graph wires via `children`,
// NOT routes. A processing/destination node's `children` (MFNode) are its INPUTS
// — the sources feeding INTO it. The graph flows children -> parent up to the
// AudioDestination output. So attach() walks the AudioDestination and recurses
// `children`, building the backend graph BOTTOM-UP: create sources first, then
// processing, connecting each child -> its parent.
//
// MAPPING (per the design):
//   AudioDestination   -> createNode(Destination, {maxChannelCount})
//   OscillatorSource   -> createNode(Oscillator,  {frequency, detune, gain})
//   BiquadFilter       -> createNode(Biquad, {frequency, q, detune, gain, type})
//   Gain               -> createNode(Gain, {gain})
//   Delay              -> createNode(Delay, {delayTime, maxDelayTime, enabled})
//   DynamicsCompressor -> createNode(Compressor, {threshold, knee, ratio,
//                          attack, release, gain, enabled})
//   for each child c of node n: connect(handle[n], handle[c])  (c feeds INTO n)
//   update(now): for each mapped node read its animatable fields -> setParam.
//
// CLASSIC Sound (§16.4.17, the Immersive profile): each Sound node gets its own
// Destination fed by an Ellipsoid Panner, fed by its `source`. The listener is
// the viewer: each tick the camera pose, and the Sound's world location and
// direction, cross as positions (ellipsoid sizes in the Sound's frame are
// scaled to world). AudioClip sources are fetched through the AssetResolver
// seam, decoded by the injected AudioDecoder, and created as Buffer nodes —
// their PCM crosses once (ADR-0050); a Pending fetch is retried each tick.
// Their playback state follows the AudioClip time lifecycle (isActive /
// isPaused, MediaTimeSystem) and pitch.
// A BufferAudioSource (§16.4.5) feeds its authored PCM `buffer` into the same
// Buffer node (SND-4): mono after averaging any extra channels, loop honored by
// the time lifecycle, playbackRate x detune captured at activation.
// A MovieTexture source uses a separately injected movie-audio decoder and
// the same Buffer path; MediaTimeSystem supplies its lifecycle and speed.
#ifndef X3D_RUNTIME_SOUND_SYSTEM_HPP
#define X3D_RUNTIME_SOUND_SYSTEM_HPP

#include "AudioBackend.hpp"
#include "AudioDecoder.hpp"
#include "AssetResolver.hpp"

#include "X3DExecutionContext.hpp"
#include "MediaTimeSystem.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/BufferAudioSource.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/Delay.hpp"
#include "x3d/nodes/DynamicsCompressor.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/ListenerPointSource.hpp"
#include "x3d/nodes/MovieTexture.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/PeriodicWave.hpp"
#include "x3d/nodes/Sound.hpp"
#include "x3d/nodes/SpatialSound.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives the §16 Sound audio graph through an abstract AudioBackend.
 * @details attach() walks an AudioDestination's `children` graph into backend
 *          nodes + connections; update() re-reads each node's animatable fields
 *          and pushes them as setParam. Inert (a clean no-op) when constructed
 *          without a backend, so the System can always be attached.
 */
class SoundSystem : public System {
public:
  /**
   * @brief Construct with the backend (null -> inert).
   * @param backend The audio DSP implementation. Shared so a test can hold it
   *        and so the System retains it for the graph's lifetime.
   */
  explicit SoundSystem(std::shared_ptr<AudioBackend> backend)
      : backend_(std::move(backend)) {}

  // --------------------------------------------------------------------------
  // System interface.
  // --------------------------------------------------------------------------

  /**
   * @brief Enroll one AudioDestination (or SpatialSound): build its whole
   *        `children` graph.
   * @details A no-op for any other node type, and inert without a backend.
   *          Recurses the AudioDestination's `children` BOTTOM-UP, creating a
   *          backend node per §16 node and connecting each child -> its parent.
   *          Records the destination handle so update()/render() can address it.
   *          SpatialSound nodes are treated as stereo destinations: an
   *          AudioDestination is synthesized and the SpatialSound's children
   *          are walked through a Panner backed with the resolved positions.
   */
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (!backend_) return;
    ctx_ = &ctx;

    if (auto *snd = dynamic_cast<x3d::nodes::Sound *>(node)) {
      // Classic Sound: Destination <- Ellipsoid Panner <- source.
      NodeParams dp;
      dp.maxChannelCount = 2;
      NodeHandle destHandle = backend_->createNode(NodeKind::Destination, dp);
      if (destHandle == kInvalidNodeHandle) return;
      destinations_.push_back(destHandle);
      rootDest_[snd] = destHandle;
      NodeHandle panner = backend_->createNode(NodeKind::Panner, soundPannerParams(snd, ctx));
      if (panner == kInvalidNodeHandle) return;
      backend_->connect(destHandle, panner);
      sounds_.push_back({snd, panner});
      if (auto src = snd->getSource()) buildChild(src.get(), panner);
      return;
    }

    if (auto *dest = dynamic_cast<x3d::nodes::AudioDestination *>(node)) {
      NodeParams dp;
      dp.maxChannelCount = dest->getMaxChannelCount();
      dp.gain = dest->getGain();
      NodeHandle destHandle = backend_->createNode(NodeKind::Destination, dp);
      if (destHandle == kInvalidNodeHandle) return;
      map_.emplace(dest, destHandle);
      rootDest_[dest] = destHandle;
      destinations_.push_back(destHandle);
      for (const auto &child : dest->getChildren())
        buildChild(child.get(), destHandle);
      return;
    }

    if (auto *ss = dynamic_cast<x3d::nodes::SpatialSound *>(node)) {
      // SpatialSound: create a synthetic Destination + Panner, wire the
      // SpatialSound's audio children through the Panner.
      NodeParams dp;
      dp.maxChannelCount = 2;
      NodeHandle destHandle = backend_->createNode(NodeKind::Destination, dp);
      if (destHandle == kInvalidNodeHandle) return;
      destinations_.push_back(destHandle);
      rootDest_[ss] = destHandle;

      // Build the Panner params (positions, listener orientation).
      // listener_ may be null (e.g. tests not providing one) -> use defaults.
      NodeParams pp = buildPannerParams(ss, listener_);
      NodeHandle pannerHandle = backend_->createNode(NodeKind::Panner, pp);
      if (pannerHandle == kInvalidNodeHandle) return;
      map_.emplace(ss, pannerHandle);
      backend_->connect(destHandle, pannerHandle);

      for (const auto &child : ss->getChildren())
        buildChild(child.get(), pannerHandle);
    }
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (auto it = rootDest_.find(node); it != rootDest_.end()) {
      destinations_.erase(std::remove(destinations_.begin(), destinations_.end(), it->second),
                          destinations_.end());
      rootDest_.erase(it);
    }
    sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
        [node](const SoundEntry &e) { return e.node == node; }), sounds_.end());
    pendingClips_.erase(std::remove_if(pendingClips_.begin(), pendingClips_.end(),
        [node](const PendingClip &e) { return e.clip == node; }), pendingClips_.end());
    pendingMovies_.erase(std::remove_if(pendingMovies_.begin(), pendingMovies_.end(),
        [node](const PendingMovie &e) { return e.movie == node; }), pendingMovies_.end());
    fallbackPitch_.erase(dynamic_cast<x3d::nodes::AudioClip *>(node));
    fallbackSpeed_.erase(dynamic_cast<x3d::nodes::MovieTexture *>(node));
    if (auto *osc = dynamic_cast<x3d::nodes::OscillatorSource *>(node))
      waves_.erase(osc);
    fallbackRate_.erase(dynamic_cast<x3d::nodes::BufferAudioSource *>(node));
    if (listener_ == node) listener_ = nullptr;
    map_.erase(node);
  }

  /**
   * @brief Register the scene's ListenerPointSource so SoundSystem can resolve
   *        listener position + orientation when building SpatialSound nodes.
   * @details Call this before attach() if the scene has a ListenerPointSource.
   *          SoundSystem does NOT take ownership — the listener's lifetime must
   *          exceed the SoundSystem's.
   */
  void setListener(x3d::nodes::ListenerPointSource *listener) { listener_ = listener; }

  /**
   * @brief Register the PeriodicWave (§16.4.18) that shapes one
   *        OscillatorSource; call again with null to unregister.
   * @details The X3D 4.0 object model gives OscillatorSource NO periodicWave
   *          field (added in 4.1), so a parsed scene's authored link cannot be
   *          read back at runtime — an embedder bridges that gap by passing
   *          the wave node here before attach(). The four standard types map
   *          onto the seam's Waveform enum; CUSTOM carries the
   *          optionsReal/optionsImag harmonic terms in the node's params.
   *          SoundSystem does no waveform math (seam purity — the backend
   *          synthesizes).
   */
  void setPeriodicWave(const x3d::nodes::OscillatorSource *osc,
                       const x3d::nodes::PeriodicWave *wave) {
    if (wave)
      waves_[osc] = wave;
    else
      waves_.erase(osc);
  }

  /** @brief Byte oracle for AudioClip urls (null -> no clip loads). */
  void setAssetResolver(extract::AssetResolver r) { resolver_ = std::move(r); }
  /** @brief AudioClip decoder (null / default -> clips stay silent). */
  void setAudioDecoder(AudioDecoder d) { decoder_ = std::move(d); }
  /** @brief MovieTexture audio decoder (null / default -> movies stay silent). */
  void setMovieAudioDecoder(AudioDecoder d) { movieAudioDecoder_ = std::move(d); }

  /**
   * @brief Render one AudioDestination's graph to a stereo interleaved buffer.
   * @details Renders the i-th enrolled destination (default the first).
   *          Inert (clears outLR) without a backend or destination.
   */
  void renderStereo(int frames, float sampleRate, std::vector<float> &outLR,
                    std::size_t destIndex = 0) {
    if (!backend_ || destIndex >= destinations_.size()) {
      outLR.clear();
      return;
    }
    backend_->renderStereo(destinations_[destIndex], frames, sampleRate, outLR);
  }

  /**
   * @brief Re-read every mapped node's animatable fields -> setParam.
   * @details Param animation rides the inbound cascade: an author routes an
   *          interpolator into (e.g.) Gain.gain; the cascade writes the field;
   *          here we read the current field each tick and push it to the backend.
   *          Idempotent and side-effect-free on the scene (read-only), so
   *          re-invocation within a tick is harmless.
   */
  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    if (!backend_) return;
    ctx_ = &ctx;
    retryPendingClips();
    retryPendingMovies();
    for (const auto &m : map_) pushParams(m.first, m.second);
    for (const SoundEntry &se : sounds_) pushSound(se, ctx);
  }

  /**
   * @brief Render one AudioDestination's graph to a mono buffer (for tests/embed).
   * @details Renders the i-th enrolled AudioDestination (default the first).
   *          Inert (clears `out`) without a backend or destination.
   */
  void render(int frames, float sampleRate, std::vector<float> &out,
              std::size_t destIndex = 0) {
    if (!backend_ || destIndex >= destinations_.size()) {
      out.clear();
      return;
    }
    backend_->render(destinations_[destIndex], frames, sampleRate, out);
  }

  /** @brief Number of §16 nodes mapped into the backend (for tests). */
  std::size_t nodeCount() const { return map_.size(); }

  /** @brief Number of AudioDestinations enrolled (for tests). */
  std::size_t destinationCount() const { return destinations_.size(); }

private:
  /**
   * @brief Create the backend node for one §16 node, recurse its children, and
   *        connect this node -> its parent.
   * @return The created handle (or kInvalidNodeHandle if the node is not a v1
   *         §16 node — it is skipped, its subtree not built).
   */
  NodeHandle buildChild(X3DNode *node, NodeHandle parent) {
    if (!node) return kInvalidNodeHandle;

    NodeHandle handle = kInvalidNodeHandle;
    if (auto *osc = dynamic_cast<x3d::nodes::OscillatorSource *>(node)) {
      NodeParams p;
      p.frequency = osc->getFrequency();
      p.detune = osc->getDetune();
      p.gain = osc->getGain();
      p.enabled = osc->getEnabled();
      p.waveform = Waveform::Sine;  // §16 default: no periodicWave -> sine
      // §16.4.18: a registered PeriodicWave shapes the oscillator. The X3D 4.0
      // bindings have no periodicWave field to read (added in 4.1 — see
      // SND-9), so the wave arrives via setPeriodicWave().
      if (const auto *w = periodicWaveFor(osc)) applyPeriodicWave(p, *w);
      handle = backend_->createNode(NodeKind::Oscillator, p);
    } else if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) {
      NodeParams p;
      p.frequency = biq->getFrequency();
      p.q = biq->getQualityFactor();
      p.detune = biq->getDetune();
      p.gain = biq->getGain();
      p.enabled = biq->getEnabled();
      p.filterType = mapFilterType(biq->getType());
      handle = backend_->createNode(NodeKind::Biquad, p);
    } else if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) {
      NodeParams p;
      p.gain = gain->getGain();
      p.enabled = gain->getEnabled();
      handle = backend_->createNode(NodeKind::Gain, p);
    } else if (auto *delay = dynamic_cast<x3d::nodes::Delay *>(node)) {
      NodeParams p;
      p.delayTime = static_cast<float>(delay->getDelayTime());
      p.maxDelayTime = static_cast<float>(delay->getMaxDelayTime());
      p.enabled = delay->getEnabled();
      handle = backend_->createNode(NodeKind::Delay, p);
    } else if (auto *cmp = dynamic_cast<x3d::nodes::DynamicsCompressor *>(node)) {
      NodeParams p;
      p.threshold = cmp->getThreshold();
      p.knee = cmp->getKnee();
      p.ratio = cmp->getRatio();
      p.attack = static_cast<float>(cmp->getAttack());
      p.release = static_cast<float>(cmp->getRelease());
      p.gain = cmp->getGain();
      p.enabled = cmp->getEnabled();
      handle = backend_->createNode(NodeKind::Compressor, p);
    } else if (auto *clip = dynamic_cast<x3d::nodes::AudioClip *>(node)) {
      // AudioClip: a Buffer node once its bytes are fetched and decoded.
      fallbackPitch_[clip] = clip->getPitch();
      const ClipLoad load = loadClip(clip, handle);
      if (load == ClipLoad::Pending) pendingClips_.push_back({clip, parent});
      if (load != ClipLoad::Ready) return kInvalidNodeHandle;
    } else if (auto *movie = dynamic_cast<x3d::nodes::MovieTexture *>(node)) {
      fallbackSpeed_[movie] = movie->getSpeed();
      const ClipLoad load = loadMovieAudio(movie, handle);
      if (load == ClipLoad::Pending) pendingMovies_.push_back({movie, parent});
      if (load != ClipLoad::Ready) return kInvalidNodeHandle;
    } else if (auto *bas = dynamic_cast<x3d::nodes::BufferAudioSource *>(node)) {
      // BufferAudioSource (§16.4.5, SND-4): the authored `buffer` MFFloat IS
      // PCM — feed the same Buffer node an AudioClip decodes into.
      fallbackRate_[bas] = bufferAudioRate(bas);
      NodeParams bp;
      bp.samples = bufferAudioSamples(bas);
      bp.sampleRate = bas->getSampleRate() > 0.0f ? bas->getSampleRate() : 44100.0f;
      bp.gain = bas->getGain();
      handle = backend_->createNode(NodeKind::Buffer, bp);
    } else if (auto *ss = dynamic_cast<SpatialSound *>(node)) {
      // SpatialSound as an audio-graph child (§16 allows nesting): insert a
      // Panner carrying the resolved positions. The SpatialSound's own
      // children (audio sources) feed into the Panner. POSITIONS cross the
      // seam; no gain is computed here (seam-purity contract).
      NodeParams pp = buildPannerParams(ss, listener_);
      handle = backend_->createNode(NodeKind::Panner, pp);
    } else {
      // Not a v1 §16 node (e.g. Convolver/Analyser/StreamAudioSource — deferred).
      // Skip its subtree; a real backend would extend the kind set here.
      return kInvalidNodeHandle;
    }
    if (handle == kInvalidNodeHandle) return kInvalidNodeHandle;

    map_.emplace(node, handle);
    // Recurse children FIRST so the whole subtree exists, then wire this node
    // INTO its parent — the graph is built bottom-up, children before parents.
    for (const auto &child : childrenOf(node))
      buildChild(child.get(), handle);
    backend_->connect(parent, handle);  // this node feeds INTO its parent
    return handle;
  }

  /** @brief The registered PeriodicWave for `osc`, or null. */
  const x3d::nodes::PeriodicWave *periodicWaveFor(
      const x3d::nodes::OscillatorSource *osc) const {
    const auto it = waves_.find(osc);
    return it == waves_.end() ? nullptr : it->second;
  }

  /** @brief Map a §16.4.18 PeriodicWave onto oscillator NodeParams. */
  static void applyPeriodicWave(NodeParams &p, const x3d::nodes::PeriodicWave &w) {
    switch (w.getType()) {
    case PeriodicWaveTypeChoices::SINE:     p.waveform = Waveform::Sine;     break;
    case PeriodicWaveTypeChoices::SQUARE:   p.waveform = Waveform::Square;   break;
    case PeriodicWaveTypeChoices::SAWTOOTH: p.waveform = Waveform::Sawtooth; break;
    case PeriodicWaveTypeChoices::TRIANGLE: p.waveform = Waveform::Triangle; break;
    case PeriodicWaveTypeChoices::CUSTOM:
      p.optionsReal = w.getOptionsReal();
      p.optionsImag = w.getOptionsImag();
      // No authored harmonics -> the sine default (§16: periodicWave NULL).
      p.waveform = p.optionsReal.empty() && p.optionsImag.empty()
                       ? Waveform::Sine
                       : Waveform::Custom;
      break;
    }
  }

  /** @brief A §16 node's `children` (its INPUTS), empty for an OscillatorSource. */
  static MFNode childrenOf(X3DNode *node) {
    if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) return biq->getChildren();
    if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) return gain->getChildren();
    if (auto *delay = dynamic_cast<x3d::nodes::Delay *>(node)) return delay->getChildren();
    if (auto *cmp = dynamic_cast<x3d::nodes::DynamicsCompressor *>(node))
      return cmp->getChildren();
    if (auto *ss = dynamic_cast<x3d::nodes::SpatialSound *>(node)) return ss->getChildren();
    return MFNode{};  // OscillatorSource is a leaf source (no inputs)
  }

  /** @brief Push a node's current animatable fields to the backend (per tick). */
  void pushParams(X3DNode *node, NodeHandle handle) {
    if (auto *osc = dynamic_cast<x3d::nodes::OscillatorSource *>(node)) {
      backend_->setParam(handle, Param::Frequency, osc->getFrequency());
      backend_->setParam(handle, Param::Detune, osc->getDetune());
      backend_->setParam(handle, Param::Gain, osc->getGain());
      backend_->setParam(handle, Param::Enabled, osc->getEnabled() ? 1.0f : 0.0f);
      pushTimeState(osc, handle);
    } else if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) {
      backend_->setParam(handle, Param::Frequency, biq->getFrequency());
      backend_->setParam(handle, Param::Q, biq->getQualityFactor());
      backend_->setParam(handle, Param::Detune, biq->getDetune());
      backend_->setParam(handle, Param::Gain, biq->getGain());
      backend_->setParam(handle, Param::Enabled, biq->getEnabled() ? 1.0f : 0.0f);
      pushTimeState(biq, handle);
    } else if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) {
      backend_->setParam(handle, Param::Gain, gain->getGain());
      backend_->setParam(handle, Param::Enabled, gain->getEnabled() ? 1.0f : 0.0f);
      pushTimeState(gain, handle);
    } else if (auto *delay = dynamic_cast<x3d::nodes::Delay *>(node)) {
      backend_->setParam(handle, Param::DelayTime,
                         static_cast<float>(delay->getDelayTime()));
      backend_->setParam(handle, Param::MaxDelayTime,
                         static_cast<float>(delay->getMaxDelayTime()));
      backend_->setParam(handle, Param::Enabled, delay->getEnabled() ? 1.0f : 0.0f);
      pushTimeState(delay, handle);
    } else if (auto *cmp = dynamic_cast<x3d::nodes::DynamicsCompressor *>(node)) {
      backend_->setParam(handle, Param::Threshold, cmp->getThreshold());
      backend_->setParam(handle, Param::Knee, cmp->getKnee());
      backend_->setParam(handle, Param::Ratio, cmp->getRatio());
      backend_->setParam(handle, Param::Attack, static_cast<float>(cmp->getAttack()));
      backend_->setParam(handle, Param::Release, static_cast<float>(cmp->getRelease()));
      backend_->setParam(handle, Param::Gain, cmp->getGain());
      backend_->setParam(handle, Param::Enabled, cmp->getEnabled() ? 1.0f : 0.0f);
      pushTimeState(cmp, handle);
    } else if (auto *clip = dynamic_cast<x3d::nodes::AudioClip *>(node)) {
      // Playback follows the §8.2.4 lifecycle outputs (MediaTimeSystem).
      const bool active = clip->X3DTimeDependentNode::getIsActive();
      const bool paused = clip->X3DTimeDependentNode::getIsPaused();
      backend_->setParam(handle, Param::PlaybackState, !active ? 0.0f : paused ? 2.0f : 1.0f);
      // §16.4.2: retain the pitch captured at activation.
      const auto *media = ctx_ ? ctx_->findSystem<MediaTimeSystem>() : nullptr;
      if (!active) fallbackPitch_[clip] = clip->getPitch();
      backend_->setParam(handle, Param::PlaybackRate,
                         media ? static_cast<float>(media->playbackRate(clip)) : fallbackPitch_[clip]);
      backend_->setParam(handle, Param::Gain, clip->getGain());
    } else if (auto *movie = dynamic_cast<x3d::nodes::MovieTexture *>(node)) {
      // §16.4.17 / §18.4.2: movie audio follows its active/paused state and activation speed.
      const bool active = movie->X3DTimeDependentNode::getIsActive();
      const bool paused = movie->X3DTimeDependentNode::getIsPaused();
      backend_->setParam(handle, Param::PlaybackState, !active ? 0.0f : paused ? 2.0f : 1.0f);
      const auto *media = ctx_ ? ctx_->findSystem<MediaTimeSystem>() : nullptr;
      if (!active) fallbackSpeed_[movie] = movie->getSpeed();
      backend_->setParam(handle, Param::PlaybackRate,
                         media ? static_cast<float>(media->playbackRate(movie)) : fallbackSpeed_[movie]);
    } else if (auto *bas = dynamic_cast<x3d::nodes::BufferAudioSource *>(node)) {
      // §16.4.5: playback follows the §8.2.4 lifecycle outputs (MediaTimeSystem);
      // playbackRate x detune is captured at activation, like AudioClip.pitch.
      const bool active = bas->X3DTimeDependentNode::getIsActive();
      const bool paused = bas->X3DTimeDependentNode::getIsPaused();
      backend_->setParam(handle, Param::PlaybackState, !active ? 0.0f : paused ? 2.0f : 1.0f);
      const auto *media = ctx_ ? ctx_->findSystem<MediaTimeSystem>() : nullptr;
      if (!active) fallbackRate_[bas] = bufferAudioRate(bas);
      backend_->setParam(handle, Param::PlaybackRate,
                         media ? static_cast<float>(media->playbackRate(bas)) : fallbackRate_[bas]);
      backend_->setParam(handle, Param::Gain, bas->getGain());
    } else if (auto *dest = dynamic_cast<x3d::nodes::AudioDestination *>(node)) {
      backend_->setParam(handle, Param::Gain, dest->getGain());
    }
  }

  void pushTimeState(X3DNode *node, NodeHandle handle) {
    if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node); gain && !gain->getEnabled()) return;
    if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node); biq && !biq->getEnabled()) return;
    if (auto *delay = dynamic_cast<x3d::nodes::Delay *>(node); delay && !delay->getEnabled()) return;
    auto *tdn = dynamic_cast<x3d::nodes::X3DTimeDependentNode *>(node);
    if (!tdn) return;
    const bool active = tdn->getIsActive();
    const bool paused = tdn->getIsPaused();
    backend_->setParam(handle, Param::PlaybackState,
                       !active ? 0.0f : paused ? 2.0f : 1.0f);
  }

  /** @brief Map the §16 BiquadTypeFilterChoices enum to the seam's FilterType. */
  static FilterType mapFilterType(BiquadTypeFilterChoices t) {
    switch (t) {
    case BiquadTypeFilterChoices::LOWPASS: return FilterType::Lowpass;
    case BiquadTypeFilterChoices::HIGHPASS: return FilterType::Highpass;
    case BiquadTypeFilterChoices::BANDPASS: return FilterType::Bandpass;
    case BiquadTypeFilterChoices::LOWSHELF: return FilterType::Lowshelf;
    case BiquadTypeFilterChoices::HIGHSHELF: return FilterType::Highshelf;
    case BiquadTypeFilterChoices::PEAKING: return FilterType::Peaking;
    case BiquadTypeFilterChoices::NOTCH: return FilterType::Notch;
    case BiquadTypeFilterChoices::ALLPASS: return FilterType::Allpass;
    }
    return FilterType::Lowpass;
  }

  /** @brief Map the §16 DistanceModelChoices enum to the seam's DistanceModel. */
  static DistanceModel mapDistanceModel(DistanceModelChoices dm) {
    switch (dm) {
    case DistanceModelChoices::LINEAR:      return DistanceModel::Linear;
    case DistanceModelChoices::INVERSE:     return DistanceModel::Inverse;
    case DistanceModelChoices::EXPONENTIAL: return DistanceModel::Exponential;
    }
    return DistanceModel::Inverse;
  }

  /**
   * @brief Build a NodeParams for a Panner from a SpatialSound + optional
   *        ListenerPointSource. POSITIONS only — no precomputed gain/pan
   *        coefficient (seam-purity contract).
   * @details This is SDK-side plumbing: read the X3D SFRotation axis-angle from
   *          the listener, apply it to the default forward (0,0,-1) and up
   *          (0,1,0) vectors, and write the resulting unit vectors into the
   *          NodeParams. All spatial DSP (distance model, azimuth, pan law)
   *          is performed INSIDE the backend from these positions.
   */
  static NodeParams buildPannerParams(x3d::nodes::SpatialSound *ss,
                                      x3d::nodes::ListenerPointSource *listener) {
    NodeParams p;

    // Source position: SpatialSound.location (SFVec3f).
    SFVec3f loc = ss->getLocation();
    p.sourcePosition[0] = loc.x;
    p.sourcePosition[1] = loc.y;
    p.sourcePosition[2] = loc.z;

    // Distance model parameters.
    p.distanceModel      = mapDistanceModel(ss->getDistanceModel());
    p.referenceDistance  = ss->getReferenceDistance();
    p.maxDistance        = ss->getMaxDistance();
    p.rolloffFactor      = ss->getRolloffFactor();

    // Listener position (defaults to origin if no listener).
    if (listener) {
      SFVec3f lpos = listener->getPosition();
      p.listenerPosition[0] = lpos.x;
      p.listenerPosition[1] = lpos.y;
      p.listenerPosition[2] = lpos.z;

      // Derive forward and up from the listener's SFRotation (axis-angle).
      // X3D spec: "orientation is rotation relative to default -Z axis direction".
      // Default forward = (0, 0, -1), default up = (0, 1, 0).
      // Apply the axis-angle rotation to derive the actual forward/up vectors.
      // This is pure plumbing — NOT spatial DSP.
      SFRotation ori = listener->getOrientation();
      double ax = static_cast<double>(ori.x);
      double ay = static_cast<double>(ori.y);
      double az = static_cast<double>(ori.z);
      double angle = static_cast<double>(ori.angle);

      // Normalize axis (guard zero-axis: identity rotation).
      double axisLen = std::sqrt(ax * ax + ay * ay + az * az);
      if (axisLen > 1e-12) {
        ax /= axisLen; ay /= axisLen; az /= axisLen;
      } else {
        // Zero axis -> identity rotation, use defaults.
        ax = 0.0; ay = 0.0; az = 1.0;  // arbitrary axis; angle will be 0
        angle = 0.0;
      }

      // Rodrigues' rotation formula: rotate vector v by (axis, angle).
      // v_rot = v*cos + (axis x v)*sin + axis*(axis.v)*(1-cos)
      auto rotVec = [&](double vx, double vy, double vz,
                        float &outX, float &outY, float &outZ) {
        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        double dot  = ax * vx + ay * vy + az * vz;
        double cx   = ay * vz - az * vy;
        double cy   = az * vx - ax * vz;
        double cz   = ax * vy - ay * vx;
        outX = static_cast<float>(vx * cosA + cx * sinA + ax * dot * (1.0 - cosA));
        outY = static_cast<float>(vy * cosA + cy * sinA + ay * dot * (1.0 - cosA));
        outZ = static_cast<float>(vz * cosA + cz * sinA + az * dot * (1.0 - cosA));
      };

      // Rotate default forward (0, 0, -1) and default up (0, 1, 0).
      rotVec(0.0, 0.0, -1.0,
             p.listenerForward[0], p.listenerForward[1], p.listenerForward[2]);
      rotVec(0.0, 1.0,  0.0,
             p.listenerUp[0], p.listenerUp[1], p.listenerUp[2]);
    }
    // If no listener: listenerPosition defaults to (0,0,0),
    // listenerForward to (0,0,-1), listenerUp to (0,1,0) — already the
    // NodeParams defaults.

    return p;
  }

  // ── classic Sound + AudioClip plumbing (positions and bytes only) ────────
  struct SoundEntry {
    x3d::nodes::Sound *node;
    NodeHandle panner;
  };
  struct PendingClip {
    x3d::nodes::AudioClip *clip;
    NodeHandle parent;
  };
  struct PendingMovie {
    x3d::nodes::MovieTexture *movie;
    NodeHandle parent;
  };
  enum class ClipLoad { Ready, Pending, Failed };

  // §16.4.5: computedPlaybackRate = playbackRate * 2^(detune/1200) (detune in
  // cents). Reversed (negative) playback is not modeled by the Buffer node.
  static float bufferAudioRate(x3d::nodes::BufferAudioSource *bas) {
    return bas->getPlaybackRate() * std::pow(2.0f, bas->getDetune() / 1200.0f);
  }

  // The Buffer node's contract is mono PCM (ADR-0050): a buffer authored with
  // numberOfChannels > 1 holds interleaved frames, so the channels are averaged
  // down (the WAV decoder's downmix policy); channelCountMode/channelInterpretation
  // are not modeled. bufferDuration limits the used portion in seconds (0 = all).
  static std::vector<float> bufferAudioSamples(x3d::nodes::BufferAudioSource *bas) {
    const MFFloat &buf = bas->getBuffer();
    std::vector<float> mono;
    const int channels = bas->getNumberOfChannels();
    if (channels > 1) {
      mono.reserve(buf.size() / static_cast<std::size_t>(channels));
      for (std::size_t frame = 0; frame + static_cast<std::size_t>(channels) <= buf.size();
           frame += static_cast<std::size_t>(channels)) {
        float sum = 0.0f;
        for (int c = 0; c < channels; ++c) sum += buf[frame + static_cast<std::size_t>(c)];
        mono.push_back(sum / static_cast<float>(channels));
      }
    } else {
      mono = buf;
    }
    if (bas->getBufferDuration() > 0.0 && bas->getSampleRate() > 0.0f) {
      const std::size_t keep = std::min(
          mono.size(), static_cast<std::size_t>(std::floor(bas->getBufferDuration() *
                                                          bas->getSampleRate())));
      mono.resize(keep);
    }
    return mono;
  }

  // Fetch the clip's urls in order and decode the first that loads. Ready
  // creates the Buffer node (its PCM crosses the seam here, once) and posts
  // duration_changed; Pending means retry next tick.
  ClipLoad loadClip(x3d::nodes::AudioClip *clip, NodeHandle &out) {
    if (!resolver_ || !decoder_) return ClipLoad::Failed;
    bool pending = false;
    for (const std::string &url : clip->getUrl()) {
      const extract::AssetResult r = resolver_(url, extract::AssetKind::Audio);
      if (r.pending()) { pending = true; break; }
      if (!r.ready()) continue;
      DecodedAudio audio = decoder_(r.bytes);
      if (!audio.ok || audio.samples.empty()) continue;
      NodeParams bp;
      bp.samples = std::move(audio.samples);
      bp.sampleRate = audio.sampleRate;
      bp.gain = clip->getGain();
      out = backend_->createNode(NodeKind::Buffer, bp);
      if (out == kInvalidNodeHandle) return ClipLoad::Failed;
      if (ctx_)
        ctx_->postEvent(clip, "duration_changed",
                        std::any(SFTime{static_cast<double>(bp.samples.size()) / bp.sampleRate}));
      return ClipLoad::Ready;
    }
    return pending ? ClipLoad::Pending : ClipLoad::Failed;
  }

  ClipLoad loadMovieAudio(x3d::nodes::MovieTexture *movie, NodeHandle &out) {
    if (!resolver_ || !movieAudioDecoder_) return ClipLoad::Failed;
    if (!movie->getLoad()) return ClipLoad::Pending;
    for (const std::string &url : movie->getUrl()) {
      const extract::AssetResult r = resolver_(url, extract::AssetKind::Movie);
      if (r.pending()) return ClipLoad::Pending;
      if (!r.ready()) continue;
      DecodedAudio audio = movieAudioDecoder_(r.bytes);
      if (!audio.ok || audio.samples.empty() || audio.sampleRate <= 0.0f) continue;
      NodeParams bp;
      bp.samples = std::move(audio.samples);
      bp.sampleRate = audio.sampleRate;
      out = backend_->createNode(NodeKind::Buffer, bp);
      return out == kInvalidNodeHandle ? ClipLoad::Failed : ClipLoad::Ready;
    }
    return ClipLoad::Failed;
  }

  void retryPendingClips() {
    for (auto it = pendingClips_.begin(); it != pendingClips_.end();) {
      NodeHandle h = kInvalidNodeHandle;
      const ClipLoad load = loadClip(it->clip, h);
      if (load == ClipLoad::Pending) { ++it; continue; }
      if (load == ClipLoad::Ready) {
        map_.emplace(it->clip, h);
        backend_->connect(it->parent, h);
      }
      it = pendingClips_.erase(it);
    }
  }

  void retryPendingMovies() {
    for (auto it = pendingMovies_.begin(); it != pendingMovies_.end();) {
      NodeHandle h = kInvalidNodeHandle;
      const ClipLoad load = loadMovieAudio(it->movie, h);
      if (load == ClipLoad::Pending) { ++it; continue; }
      if (load == ClipLoad::Ready) {
        map_.emplace(it->movie, h);
        backend_->connect(it->parent, h);
      }
      it = pendingMovies_.erase(it);
    }
  }

  // The Sound's world location / direction and the viewer pose, as geometry.
  // Ellipsoid lengths are in the Sound's frame: scale them to world.
  static NodeParams soundPannerParams(x3d::nodes::Sound *snd, X3DExecutionContext &ctx) {
    NodeParams p;
    p.distanceModel = DistanceModel::Ellipsoid;
    const Mat4 w = ctx.worldTransformAny(snd);
    const SFVec3f loc = w.transformPoint(snd->getLocation());
    const SFVec3f dir = w.transformDirection(snd->getDirection());
    const SFVec3f unitX = w.transformDirection(SFVec3f{1, 0, 0});
    const float scale = std::sqrt(unitX.x * unitX.x + unitX.y * unitX.y + unitX.z * unitX.z);
    const float s = scale > 0.0f ? scale : 1.0f;
    p.sourcePosition[0] = loc.x; p.sourcePosition[1] = loc.y; p.sourcePosition[2] = loc.z;
    p.direction[0] = dir.x; p.direction[1] = dir.y; p.direction[2] = dir.z;
    p.minFront = snd->getMinFront() * s;
    p.minBack = snd->getMinBack() * s;
    p.maxFront = snd->getMaxFront() * s;
    p.maxBack = snd->getMaxBack() * s;
    p.intensity = snd->getIntensity();
    p.spatialize = snd->getSpatialize();
    const SFVec3f eye = ctx.cameraWorldPosition();
    const Mat4 camToWorld = ctx.viewMatrix().inverse();
    const SFVec3f fwd = camToWorld.transformDirection(SFVec3f{0, 0, -1});
    const SFVec3f up = camToWorld.transformDirection(SFVec3f{0, 1, 0});
    p.listenerPosition[0] = eye.x; p.listenerPosition[1] = eye.y; p.listenerPosition[2] = eye.z;
    p.listenerForward[0] = fwd.x; p.listenerForward[1] = fwd.y; p.listenerForward[2] = fwd.z;
    p.listenerUp[0] = up.x; p.listenerUp[1] = up.y; p.listenerUp[2] = up.z;
    return p;
  }

  void pushSound(const SoundEntry &se, X3DExecutionContext &ctx) {
    const NodeParams p = soundPannerParams(se.node, ctx);
    const NodeHandle h = se.panner;
    backend_->setParam(h, Param::PositionX, p.sourcePosition[0]);
    backend_->setParam(h, Param::PositionY, p.sourcePosition[1]);
    backend_->setParam(h, Param::PositionZ, p.sourcePosition[2]);
    backend_->setParam(h, Param::DirectionX, p.direction[0]);
    backend_->setParam(h, Param::DirectionY, p.direction[1]);
    backend_->setParam(h, Param::DirectionZ, p.direction[2]);
    backend_->setParam(h, Param::ListenerPositionX, p.listenerPosition[0]);
    backend_->setParam(h, Param::ListenerPositionY, p.listenerPosition[1]);
    backend_->setParam(h, Param::ListenerPositionZ, p.listenerPosition[2]);
    backend_->setParam(h, Param::ListenerForwardX, p.listenerForward[0]);
    backend_->setParam(h, Param::ListenerForwardY, p.listenerForward[1]);
    backend_->setParam(h, Param::ListenerForwardZ, p.listenerForward[2]);
    backend_->setParam(h, Param::ListenerUpX, p.listenerUp[0]);
    backend_->setParam(h, Param::ListenerUpY, p.listenerUp[1]);
    backend_->setParam(h, Param::ListenerUpZ, p.listenerUp[2]);
    backend_->setParam(h, Param::Intensity, p.intensity);
  }

  std::vector<SoundEntry> sounds_;
  std::vector<PendingClip> pendingClips_;
  std::vector<PendingMovie> pendingMovies_;
  std::unordered_map<x3d::nodes::AudioClip *, float> fallbackPitch_;
  std::unordered_map<x3d::nodes::MovieTexture *, float> fallbackSpeed_;
  // OscillatorSource -> its registered §16.4.18 PeriodicWave (borrowed; the
  // wave node must outlive the System). Read at attach() time only.
  std::unordered_map<const x3d::nodes::OscillatorSource *,
                     const x3d::nodes::PeriodicWave *>
      waves_;
  std::unordered_map<x3d::nodes::BufferAudioSource *, float> fallbackRate_;
  extract::AssetResolver resolver_;
  AudioDecoder decoder_ = makeNullAudioDecoder();
  AudioDecoder movieAudioDecoder_;
  X3DExecutionContext *ctx_ = nullptr;

  std::shared_ptr<AudioBackend> backend_;
  // §16 node -> backend handle. unordered_map iteration order is unspecified, so
  // update() must stay order-independent (setParam is idempotent per node — it
  // is). Graph CONSTRUCTION order (attach) is deterministic: it follows the
  // children recursion, not this map.
  std::unordered_map<X3DNode *, NodeHandle> map_;
  std::unordered_map<X3DNode *, NodeHandle> rootDest_;
  std::vector<NodeHandle> destinations_;
  // Optional listener (null if scene has none). Resolved to forward/up vectors
  // in buildPannerParams (plumbing only — no spatial DSP here).
  x3d::nodes::ListenerPointSource *listener_ = nullptr;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SOUND_SYSTEM_HPP
