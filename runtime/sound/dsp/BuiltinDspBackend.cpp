// BuiltinDspBackend.cpp — the ONLY translation unit with DSP in the sound seam.
// All audio math (oscillator phase accumulators, RBJ "Audio EQ Cookbook" biquad
// coefficients + per-sample difference equation + filter state, gain multiply,
// destination summing, the topological render pull) lives here. The seam
// (AudioBackend.hpp) and the System (SoundSystem.hpp) stay DSP-free.
//
// Rendering model (mono, v1): each node is pulled for `frames` samples per
// render() call. render() walks the destination's inputs depth-first; each node
// computes its block from its inputs' blocks (sources generate; biquads filter;
// gains scale; the destination sums). Per-call block processing keeps a node's
// own state (oscillator phase, biquad history) so successive render() calls on a
// persistent backend continue the waveform seamlessly — and a fresh backend with
// the same graph + params renders byte-identically (determinism).

#include "BuiltinDspBackend.hpp"

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

// Effective frequency after applying detune in cents: f * 2^(detune/1200).
inline double detuned(double freqHz, double detuneCents) {
  return freqHz * std::pow(2.0, detuneCents / 1200.0);
}

// One backend node. A tagged record: which kind, its params, its input handles,
// and its persistent DSP state (phase for oscillators; the 2-pole/2-zero history
// + cached coefficients for biquads). Per-render scratch (the block + a
// visited/rendered flag) is reset each render() pass.
struct Node {
  NodeKind kind = NodeKind::Oscillator;
  NodeParams params;
  std::vector<NodeHandle> inputs;

  // Oscillator state.
  double phase = 0.0;  // radians, accumulated across render() calls

  // Biquad state (Direct Form I): input/output history.
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;

  // Per-render scratch.
  std::vector<float> block;
  bool rendered = false;
};

// Naive band-unlimited waveforms (v1): adequate for the validation asserts
// (tone presence, filtering, gain). Anti-aliasing (PolyBLEP/wavetables) is a
// clean later refinement and does not change the seam.
inline double oscSample(Waveform w, double phase) {
  // phase in [0, 2pi).
  switch (w) {
  case Waveform::Sine:
    return std::sin(phase);
  case Waveform::Square:
    return phase < kPi ? 1.0 : -1.0;
  case Waveform::Sawtooth:
    return 2.0 * (phase / kTwoPi) - 1.0;  // -1..+1 ramp
  case Waveform::Triangle: {
    double t = phase / kTwoPi;  // 0..1
    return 4.0 * std::fabs(t - 0.5) - 1.0;  // /\ shape, -1..+1
  }
  }
  return 0.0;
}

// RBJ "Audio EQ Cookbook" biquad coefficients (normalized by a0). Implemented
// for LOWPASS + HIGHPASS (the §16 v1 requirement); BANDPASS + NOTCH + ALLPASS
// included; the shelf/peaking types (which need a dB gain param the seam does
// not yet carry) fall back to a pass-through (b0=1) — a named extension point.
struct BiquadCoeffs {
  double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

BiquadCoeffs computeBiquad(FilterType type, double freqHz, double q,
                           double detuneCents, double sampleRate) {
  BiquadCoeffs c;
  if (sampleRate <= 0.0) return c;
  double f0 = detuned(freqHz, detuneCents);
  // Clamp the cutoff into a sane open interval below Nyquist.
  double nyq = sampleRate * 0.5;
  if (f0 < 1.0) f0 = 1.0;
  if (f0 > nyq * 0.999) f0 = nyq * 0.999;
  if (q < 1e-4) q = 1e-4;

  double w0 = kTwoPi * f0 / sampleRate;
  double cosw0 = std::cos(w0);
  double sinw0 = std::sin(w0);
  double alpha = sinw0 / (2.0 * q);

  double a0 = 1.0 + alpha;
  double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

  switch (type) {
  case FilterType::Lowpass:
    b0 = (1.0 - cosw0) / 2.0;
    b1 = 1.0 - cosw0;
    b2 = (1.0 - cosw0) / 2.0;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;
    break;
  case FilterType::Highpass:
    b0 = (1.0 + cosw0) / 2.0;
    b1 = -(1.0 + cosw0);
    b2 = (1.0 + cosw0) / 2.0;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;
    break;
  case FilterType::Bandpass:  // constant 0 dB peak gain
    b0 = alpha;
    b1 = 0.0;
    b2 = -alpha;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;
    break;
  case FilterType::Notch:
    b0 = 1.0;
    b1 = -2.0 * cosw0;
    b2 = 1.0;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;
    break;
  case FilterType::Allpass:
    b0 = 1.0 - alpha;
    b1 = -2.0 * cosw0;
    b2 = 1.0 + alpha;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;
    break;
  case FilterType::Lowshelf:
  case FilterType::Highshelf:
  case FilterType::Peaking:
    // Need a dB gain parameter the seam does not yet carry -> pass-through.
    return BiquadCoeffs{1, 0, 0, 0, 0};
  }

  c.b0 = b0 / a0;
  c.b1 = b1 / a0;
  c.b2 = b2 / a0;
  c.a1 = a1 / a0;
  c.a2 = a2 / a0;
  return c;
}

} // namespace

struct BuiltinDspBackend::Impl {
  std::unordered_map<NodeHandle, Node> nodes;
  NodeHandle lastHandle = kInvalidNodeHandle;

  // Render a node's block into node.block (size = frames), recursing inputs.
  // Each node is rendered at most once per render() pass (the `rendered` flag),
  // so a fan-in graph (shared input) is pulled once and reused.
  void renderNode(NodeHandle h, int frames, double sampleRate) {
    auto it = nodes.find(h);
    if (it == nodes.end()) return;
    Node &n = it->second;
    if (n.rendered) return;
    n.rendered = true;
    n.block.assign(static_cast<std::size_t>(frames), 0.0f);

    switch (n.kind) {
    case NodeKind::Oscillator: {
      double f = detuned(n.params.frequency, n.params.detune);
      double inc = kTwoPi * f / sampleRate;
      double g = n.params.gain;
      for (int i = 0; i < frames; ++i) {
        n.block[static_cast<std::size_t>(i)] =
            static_cast<float>(oscSample(n.params.waveform, n.phase) * g);
        n.phase += inc;
        if (n.phase >= kTwoPi) n.phase -= kTwoPi;
      }
      break;
    }
    case NodeKind::Biquad: {
      // Sum inputs, then filter the summed signal.
      std::vector<float> in(static_cast<std::size_t>(frames), 0.0f);
      sumInputs(n, in, frames, sampleRate);
      BiquadCoeffs c =
          computeBiquad(n.params.filterType, n.params.frequency, n.params.q,
                        n.params.detune, sampleRate);
      double g = n.params.gain;
      for (int i = 0; i < frames; ++i) {
        double x0 = in[static_cast<std::size_t>(i)];
        double y0 = c.b0 * x0 + c.b1 * n.x1 + c.b2 * n.x2 -
                    c.a1 * n.y1 - c.a2 * n.y2;
        n.x2 = n.x1;
        n.x1 = x0;
        n.y2 = n.y1;
        n.y1 = y0;
        n.block[static_cast<std::size_t>(i)] = static_cast<float>(y0 * g);
      }
      break;
    }
    case NodeKind::Gain: {
      std::vector<float> in(static_cast<std::size_t>(frames), 0.0f);
      sumInputs(n, in, frames, sampleRate);
      double g = n.params.gain;
      for (int i = 0; i < frames; ++i)
        n.block[static_cast<std::size_t>(i)] =
            static_cast<float>(in[static_cast<std::size_t>(i)] * g);
      break;
    }
    case NodeKind::Destination: {
      sumInputs(n, n.block, frames, sampleRate);
      break;
    }
    case NodeKind::Panner: {
      // Task 3: equal-power spatial DSP. All computation is BACKEND-SIDE.
      // POSITIONS cross the seam (NodeParams); no precomputed gain/coefficient
      // may arrive from SoundSystem (seam-purity contract in AudioBackend.hpp).
      //
      // The rendering model (per the seam spec + X3D §16):
      //   1. Render the mono input chain into `in`.
      //   2. Compute distance d = |sourcePosition - listenerPosition|.
      //   3. Apply the chosen DistanceModel to get distGain.
      //   4. Project the source direction into the listener's right-ear frame.
      //   5. Equal-power pan: theta = (az_norm*0.5+0.5)*(pi/2);
      //      gL = cos(theta), gR = sin(theta).
      //   6. Write interleaved output: L = mono*distGain*gL, R = mono*distGain*gR.
      //      (renderStereo reads this block as L/R pairs: block[2i]=L, block[2i+1]=R.)
      //
      // Degenerate d=0 (source == listener): az_norm = 0 -> centered pan.
      std::vector<float> in(static_cast<std::size_t>(frames), 0.0f);
      sumInputs(n, in, frames, sampleRate);

      const NodeParams &np = n.params;

      // ── 1. Distance ──────────────────────────────────────────────────────────
      double dx = static_cast<double>(np.sourcePosition[0]) -
                  static_cast<double>(np.listenerPosition[0]);
      double dy = static_cast<double>(np.sourcePosition[1]) -
                  static_cast<double>(np.listenerPosition[1]);
      double dz = static_cast<double>(np.sourcePosition[2]) -
                  static_cast<double>(np.listenerPosition[2]);
      double d = std::sqrt(dx * dx + dy * dy + dz * dz);

      // Defensive: referenceDistance <= 0 would produce NaN in the inverse
      // and exponential formulae (division by zero / 0^-rolloff).  Clamp to
      // a small positive floor so the math stays well-defined.
      double ref    = static_cast<double>(np.referenceDistance);
      if (ref <= 0.0) ref = 1e-6;
      double maxD   = static_cast<double>(np.maxDistance);
      double rollof = static_cast<double>(np.rolloffFactor);

      // ── 2. Distance gain ─────────────────────────────────────────────────────
      double distGain = 1.0;
      switch (np.distanceModel) {
      case DistanceModel::Linear: {
        // gain = 1 - rolloff * (clamp(d, ref, max) - ref) / (max - ref)
        // clamped to [0, 1].
        double dClamped = d < ref ? ref : (d > maxD ? maxD : d);
        double denom = maxD - ref;
        if (denom > 1e-12)
          distGain = 1.0 - rollof * (dClamped - ref) / denom;
        else
          distGain = 1.0;
        if (distGain < 0.0) distGain = 0.0;
        break;
      }
      case DistanceModel::Inverse: {
        // gain = ref / (ref + rolloff * (max(d, ref) - ref))
        double dEffective = d < ref ? ref : d;
        double denom = ref + rollof * (dEffective - ref);
        distGain = (denom > 1e-12) ? ref / denom : 1.0;
        break;
      }
      case DistanceModel::Exponential: {
        // gain = (max(d, ref) / ref) ^ (-rolloff)
        double dEffective = d < ref ? ref : d;
        distGain = std::pow(dEffective / ref, -rollof);
        break;
      }
      }

      // ── 3. Azimuth in listener frame ─────────────────────────────────────────
      // right = normalize(cross(listenerForward, listenerUp))
      double fX = static_cast<double>(np.listenerForward[0]);
      double fY = static_cast<double>(np.listenerForward[1]);
      double fZ = static_cast<double>(np.listenerForward[2]);
      double uX = static_cast<double>(np.listenerUp[0]);
      double uY = static_cast<double>(np.listenerUp[1]);
      double uZ = static_cast<double>(np.listenerUp[2]);

      // cross(forward, up) = right ear direction
      double rX = fY * uZ - fZ * uY;
      double rY = fZ * uX - fX * uZ;
      double rZ = fX * uY - fY * uX;
      double rLen = std::sqrt(rX * rX + rY * rY + rZ * rZ);

      double azNorm = 0.0;  // default: centered (degenerate or d=0)
      if (rLen > 1e-12 && d > 1e-12) {
        // Normalize right vector and source direction
        rX /= rLen; rY /= rLen; rZ /= rLen;
        double srcX = dx / d, srcY = dy / d, srcZ = dz / d;
        // Azimuth normal = dot(srcDir, right), clamped to [-1, 1]
        azNorm = rX * srcX + rY * srcY + rZ * srcZ;
        if (azNorm < -1.0) azNorm = -1.0;
        if (azNorm >  1.0) azNorm =  1.0;
      }

      // ── 4. Equal-power pan ───────────────────────────────────────────────────
      // theta in [0, pi/2]: 0 = full left, pi/4 = center, pi/2 = full right
      double theta = (azNorm * 0.5 + 0.5) * (kPi / 2.0);
      double gL = std::cos(theta);
      double gR = std::sin(theta);

      // ── 5. Output: interleaved L/R (block is 2*frames for Panner) ───────────
      n.block.resize(static_cast<std::size_t>(frames) * 2);
      for (int i = 0; i < frames; ++i) {
        float mono = in[static_cast<std::size_t>(i)];
        n.block[static_cast<std::size_t>(i) * 2]     =
            static_cast<float>(mono * distGain * gL);
        n.block[static_cast<std::size_t>(i) * 2 + 1] =
            static_cast<float>(mono * distGain * gR);
      }
      break;
    }
    }
  }

  // Render every input of `n` and sum their blocks into `out`.
  void sumInputs(Node &n, std::vector<float> &out, int frames,
                 double sampleRate) {
    for (NodeHandle src : n.inputs) {
      renderNode(src, frames, sampleRate);
      auto it = nodes.find(src);
      if (it == nodes.end()) continue;
      const std::vector<float> &b = it->second.block;
      std::size_t m = b.size();
      for (int i = 0; i < frames && static_cast<std::size_t>(i) < m; ++i)
        out[static_cast<std::size_t>(i)] += b[static_cast<std::size_t>(i)];
    }
  }
};

BuiltinDspBackend::BuiltinDspBackend() : impl_(std::make_unique<Impl>()) {}
BuiltinDspBackend::~BuiltinDspBackend() = default;

NodeHandle BuiltinDspBackend::createNode(NodeKind kind,
                                         const NodeParams &params) {
  NodeHandle h = ++impl_->lastHandle;
  Node n;
  n.kind = kind;
  n.params = params;
  impl_->nodes.emplace(h, std::move(n));
  return h;
}

void BuiltinDspBackend::connect(NodeHandle dst, NodeHandle src) {
  auto it = impl_->nodes.find(dst);
  if (it == impl_->nodes.end()) return;
  it->second.inputs.push_back(src);
}

void BuiltinDspBackend::setParam(NodeHandle node, Param param, float value) {
  auto it = impl_->nodes.find(node);
  if (it == impl_->nodes.end()) return;
  switch (param) {
  case Param::Frequency:  it->second.params.frequency          = value; break;
  case Param::Detune:     it->second.params.detune             = value; break;
  case Param::Q:          it->second.params.q                  = value; break;
  case Param::Gain:       it->second.params.gain               = value; break;
  case Param::PositionX:  it->second.params.sourcePosition[0]  = value; break;
  case Param::PositionY:  it->second.params.sourcePosition[1]  = value; break;
  case Param::PositionZ:  it->second.params.sourcePosition[2]  = value; break;
  }
}

void BuiltinDspBackend::render(NodeHandle destination, int frames,
                               float sampleRate, std::vector<float> &out) {
  if (frames < 0) frames = 0;
  out.assign(static_cast<std::size_t>(frames), 0.0f);
  if (sampleRate <= 0.0f) return;

  for (auto &kv : impl_->nodes) kv.second.rendered = false;
  impl_->renderNode(destination, frames, static_cast<double>(sampleRate));

  auto it = impl_->nodes.find(destination);
  if (it == impl_->nodes.end()) return;
  const std::vector<float> &b = it->second.block;
  for (int i = 0; i < frames && static_cast<std::size_t>(i) < b.size(); ++i)
    out[static_cast<std::size_t>(i)] = b[static_cast<std::size_t>(i)];
}

void BuiltinDspBackend::renderStereo(NodeHandle destination, int frames,
                                      float sampleRate,
                                      std::vector<float> &outLR) {
  // Task 3: stereo render. If the destination graph contains any Panner node,
  // the Panner renders an interleaved L/R block (2*frames); the destination
  // sums those stereo pairs. Non-Panner nodes produce mono blocks (frames).
  //
  // Strategy: render the whole graph via renderNode (which applies equal-power
  // pan for Panner nodes and writes a 2*frames block), then do a stereo-aware
  // sum at the destination — for each input of the destination, if it's a
  // Panner (block.size() == 2*frames) treat it as interleaved L/R; otherwise
  // duplicate the mono sample to both channels.
  if (frames < 0) frames = 0;
  outLR.assign(static_cast<std::size_t>(frames) * 2, 0.0f);
  if (sampleRate <= 0.0f) return;

  // Reset the rendered flags for this pass.
  for (auto &kv : impl_->nodes) kv.second.rendered = false;

  // Render the destination node's subtree using renderNode().
  // For Panner nodes the block is now 2*frames (interleaved L/R).
  // For all other node kinds the block is `frames` mono samples.
  // renderNode() on the Destination itself will call sumInputs() which
  // operates mono-style (adding scalar samples). We bypass that for stereo:
  // we render the INPUTS of the destination ourselves, then do a stereo-aware
  // summation below.
  auto destIt = impl_->nodes.find(destination);
  if (destIt == impl_->nodes.end()) return;

  // Render each direct input of the destination (not the destination itself,
  // since we need to do a stereo-aware sum of those inputs).
  for (NodeHandle src : destIt->second.inputs)
    impl_->renderNode(src, frames, static_cast<double>(sampleRate));

  // Stereo-aware summation: for each input, if it's a Panner (block is
  // 2*frames), read as interleaved L/R; otherwise duplicate the mono sample.
  for (NodeHandle src : destIt->second.inputs) {
    auto it = impl_->nodes.find(src);
    if (it == impl_->nodes.end()) continue;
    const std::vector<float> &b = it->second.block;
    bool isStereoBlock = (b.size() == static_cast<std::size_t>(frames) * 2);
    if (isStereoBlock) {
      // Interleaved L/R from a Panner node.
      for (int i = 0; i < frames; ++i) {
        outLR[static_cast<std::size_t>(i) * 2]     += b[static_cast<std::size_t>(i) * 2];
        outLR[static_cast<std::size_t>(i) * 2 + 1] += b[static_cast<std::size_t>(i) * 2 + 1];
      }
    } else {
      // Mono input: duplicate to both channels.
      for (int i = 0; i < frames && static_cast<std::size_t>(i) < b.size(); ++i) {
        outLR[static_cast<std::size_t>(i) * 2]     += b[static_cast<std::size_t>(i)];
        outLR[static_cast<std::size_t>(i) * 2 + 1] += b[static_cast<std::size_t>(i)];
      }
    }
  }
}

} // namespace x3d::runtime
