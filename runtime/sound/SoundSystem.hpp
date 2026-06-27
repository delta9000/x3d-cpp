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
//   for each child c of node n: connect(handle[n], handle[c])  (c feeds INTO n)
//   update(now): for each mapped node read its animatable fields -> setParam.
#ifndef X3D_RUNTIME_SOUND_SYSTEM_HPP
#define X3D_RUNTIME_SOUND_SYSTEM_HPP

#include "AudioBackend.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/ListenerPointSource.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/SpatialSound.hpp"

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
    (void)ctx;
    if (!backend_) return;

    if (auto *dest = dynamic_cast<x3d::nodes::AudioDestination *>(node)) {
      NodeParams dp;
      dp.maxChannelCount = dest->getMaxChannelCount();
      NodeHandle destHandle = backend_->createNode(NodeKind::Destination, dp);
      if (destHandle == kInvalidNodeHandle) return;
      map_.emplace(dest, destHandle);
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

  /**
   * @brief Register the scene's ListenerPointSource so SoundSystem can resolve
   *        listener position + orientation when building SpatialSound nodes.
   * @details Call this before attach() if the scene has a ListenerPointSource.
   *          SoundSystem does NOT take ownership — the listener's lifetime must
   *          exceed the SoundSystem's.
   */
  void setListener(x3d::nodes::ListenerPointSource *listener) { listener_ = listener; }

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
    (void)ctx;
    if (!backend_) return;
    for (const auto &m : map_) pushParams(m.first, m.second);
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
      p.waveform = Waveform::Sine;  // §16 OscillatorSource has no authored type
      handle = backend_->createNode(NodeKind::Oscillator, p);
    } else if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) {
      NodeParams p;
      p.frequency = biq->getFrequency();
      p.q = biq->getQualityFactor();
      p.detune = biq->getDetune();
      p.gain = biq->getGain();
      p.filterType = mapFilterType(biq->getType());
      handle = backend_->createNode(NodeKind::Biquad, p);
    } else if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) {
      NodeParams p;
      p.gain = gain->getGain();
      handle = backend_->createNode(NodeKind::Gain, p);
    } else if (auto *ss = dynamic_cast<SpatialSound *>(node)) {
      // SpatialSound as an audio-graph child (§16 allows nesting): insert a
      // Panner carrying the resolved positions. The SpatialSound's own
      // children (audio sources) feed into the Panner. POSITIONS cross the
      // seam; no gain is computed here (seam-purity contract).
      NodeParams pp = buildPannerParams(ss, listener_);
      handle = backend_->createNode(NodeKind::Panner, pp);
    } else {
      // Not a v1 §16 node (e.g. Convolver/Delay/AudioClip — deferred). Skip its
      // subtree; a real backend would extend the kind set here.
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

  /** @brief A §16 node's `children` (its INPUTS), empty for an OscillatorSource. */
  static MFNode childrenOf(X3DNode *node) {
    if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) return biq->getChildren();
    if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) return gain->getChildren();
    if (auto *ss = dynamic_cast<x3d::nodes::SpatialSound *>(node)) return ss->getChildren();
    return MFNode{};  // OscillatorSource is a leaf source (no inputs)
  }

  /** @brief Push a node's current animatable fields to the backend (per tick). */
  void pushParams(X3DNode *node, NodeHandle handle) {
    if (auto *osc = dynamic_cast<x3d::nodes::OscillatorSource *>(node)) {
      backend_->setParam(handle, Param::Frequency, osc->getFrequency());
      backend_->setParam(handle, Param::Detune, osc->getDetune());
      backend_->setParam(handle, Param::Gain, osc->getGain());
    } else if (auto *biq = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) {
      backend_->setParam(handle, Param::Frequency, biq->getFrequency());
      backend_->setParam(handle, Param::Q, biq->getQualityFactor());
      backend_->setParam(handle, Param::Detune, biq->getDetune());
      backend_->setParam(handle, Param::Gain, biq->getGain());
    } else if (auto *gain = dynamic_cast<x3d::nodes::Gain *>(node)) {
      backend_->setParam(handle, Param::Gain, gain->getGain());
    }
    // AudioDestination has no per-tick animatable scalar in v1.
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

  std::shared_ptr<AudioBackend> backend_;
  // §16 node -> backend handle. unordered_map iteration order is unspecified, so
  // update() must stay order-independent (setParam is idempotent per node — it
  // is). Graph CONSTRUCTION order (attach) is deterministic: it follows the
  // children recursion, not this map.
  std::unordered_map<X3DNode *, NodeHandle> map_;
  std::vector<NodeHandle> destinations_;
  // Optional listener (null if scene has none). Resolved to forward/up vectors
  // in buildPannerParams (plumbing only — no spatial DSP here).
  x3d::nodes::ListenerPointSource *listener_ = nullptr;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SOUND_SYSTEM_HPP
