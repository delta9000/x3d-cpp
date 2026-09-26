// immersive_fixtures.hpp — backend-level fixtures for the Immersive Sound seam
// extension (ADR-0050): the Sound ellipsoid (DistanceModel::Ellipsoid) and the
// decoded-PCM Buffer node. Each fixture drives a bare AudioBackend, so the same
// numbers are checked on the built-in backend (sound_immersive_test) and on
// both backends side by side (sound_swap_test).
#ifndef X3D_SOUND_TESTS_IMMERSIVE_FIXTURES_HPP
#define X3D_SOUND_TESTS_IMMERSIVE_FIXTURES_HPP

#include "AudioBackend.hpp"
#include "tests/dsp_metrics.hpp"

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

namespace x3d::test {

using x3d::runtime::AudioBackend;
using x3d::runtime::DistanceModel;
using x3d::runtime::NodeHandle;
using x3d::runtime::NodeKind;
using x3d::runtime::NodeParams;
using x3d::runtime::Param;

inline constexpr float kImmSR = 48000.0f;
inline constexpr int kImmFrames = 4800;

// A Sound-shaped ellipsoid (source at the origin facing +Z, front 1..10, back
// 1..4) heard from `listenerZ` on the Z axis, listener facing -Z.
inline NodeParams ellipsoidParams(float listenerZ, bool spatialize = true) {
  NodeParams p;
  p.distanceModel = DistanceModel::Ellipsoid;
  p.direction[0] = 0; p.direction[1] = 0; p.direction[2] = 1;
  p.minFront = 1; p.maxFront = 10; p.minBack = 1; p.maxBack = 4;
  p.listenerPosition[2] = listenerZ;
  p.listenerForward[0] = 0; p.listenerForward[1] = 0; p.listenerForward[2] = -1;
  p.spatialize = spatialize;
  return p;
}

// Oscillator(440) -> Panner(p) -> stereo Destination; total stereo RMS.
inline double ellipsoidRms(const std::shared_ptr<AudioBackend> &be, const NodeParams &p) {
  NodeParams op; op.frequency = 440; op.gain = 1;
  const NodeHandle osc = be->createNode(NodeKind::Oscillator, op);
  const NodeHandle pan = be->createNode(NodeKind::Panner, p);
  NodeParams dp; dp.maxChannelCount = 2;
  const NodeHandle dst = be->createNode(NodeKind::Destination, dp);
  be->connect(pan, osc);
  be->connect(dst, pan);
  std::vector<float> lr;
  be->renderStereo(dst, kImmFrames, kImmSR, lr);
  return rmsStereo(lr);
}

// One second of a 1 kHz sine at 8 kHz — the decoded-PCM payload.
inline NodeParams toneBufferParams() {
  NodeParams p;
  p.sampleRate = 8000.0f;
  p.samples.resize(8000);
  for (std::size_t i = 0; i < p.samples.size(); ++i)
    p.samples[i] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 1000.0 * double(i) / 8000.0));
  return p;
}

struct BufferRig {
  std::shared_ptr<AudioBackend> be;
  NodeHandle buf = 0, dst = 0;
  std::vector<float> render() {
    std::vector<float> out;
    be->render(dst, kImmFrames, kImmSR, out);
    return out;
  }
};

inline BufferRig bufferRig(const std::shared_ptr<AudioBackend> &be) {
  BufferRig r{be};
  r.buf = be->createNode(NodeKind::Buffer, toneBufferParams());
  NodeParams dp; dp.maxChannelCount = 1;
  r.dst = be->createNode(NodeKind::Destination, dp);
  be->connect(r.dst, r.buf);
  return r;
}

// The expected §16.4.17 gain between the ellipsoids: -20 dB * (d-rMin)/(rMax-rMin).
inline double ellipsoidExpected(double d, double rMin, double rMax) {
  return std::pow(10.0, -(d - rMin) / (rMax - rMin));
}

// Runs the shared assertions against one backend; `check(cond, msg)` records.
inline void runImmersiveFixtures(const std::function<std::shared_ptr<AudioBackend>()> &make,
                                 const std::function<void(bool, const char *)> &check) {
  // Ellipsoid zones, as ratios to the inside level (the pan law cancels).
  const double inside = ellipsoidRms(make(), ellipsoidParams(0.5f));
  const double between = ellipsoidRms(make(), ellipsoidParams(5.0f));
  const double outside = ellipsoidRms(make(), ellipsoidParams(12.0f));
  const double behindIn = ellipsoidRms(make(), ellipsoidParams(-0.5f));
  const double behindOut = ellipsoidRms(make(), ellipsoidParams(-5.0f));
  check(inside > 0.05, "ellipsoid: inside the inner ellipsoid is audible");
  const double want = ellipsoidExpected(5.0, 1.0, 10.0);  // -8.89 dB
  check(std::fabs(between / inside - want) < 0.03 * want,
        "ellipsoid: between the ellipsoids is -20dB*(d-rMin)/(rMax-rMin)");
  check(outside < 1e-6, "ellipsoid: beyond the outer ellipsoid is silent");
  check(std::fabs(behindIn - inside) < 0.03 * inside, "ellipsoid: behind, inside minBack is full level");
  check(behindOut < 1e-6, "ellipsoid: behind, beyond maxBack (4) is silent though maxFront is 10");
  NodeParams quiet = ellipsoidParams(0.5f);
  quiet.intensity = 0.25f;
  check(std::fabs(ellipsoidRms(make(), quiet) / inside - 0.25) < 0.01, "ellipsoid: intensity scales");

  // Non-spatialized: equal channels.
  {
    auto be = make();
    NodeParams p = ellipsoidParams(0.5f, false);
    p.listenerForward[0] = 1; p.listenerForward[2] = 0;  // source hard left when spatialized
    NodeParams op; op.frequency = 440; op.gain = 1;
    const NodeHandle osc = be->createNode(NodeKind::Oscillator, op);
    const NodeHandle pan = be->createNode(NodeKind::Panner, p);
    NodeParams dp; dp.maxChannelCount = 2;
    const NodeHandle dst = be->createNode(NodeKind::Destination, dp);
    be->connect(pan, osc);
    be->connect(dst, pan);
    std::vector<float> lr;
    be->renderStereo(dst, kImmFrames, kImmSR, lr);
    const double l = rmsL(lr), r = rmsR(lr);
    check(l > 0.05 && std::fabs(l - r) < 0.01 * l, "ellipsoid: spatialize FALSE plays centred");
  }

  // Buffer: silent until played, 1 kHz at the output rate, pitch doubles it,
  // pause holds silence, stop rewinds.
  {
    BufferRig rig = bufferRig(make());
    check(rms(rig.render()) < 1e-6, "buffer: stopped by default");
    rig.be->setParam(rig.buf, Param::PlaybackState, 1);
    const std::vector<float> a = rig.render();
    const double r = rms(a);
    check(std::fabs(r - 0.5 / std::sqrt(2.0)) < 0.02, "buffer: plays the decoded PCM at its level");
    check(goertzel(a, 1000.0, kImmSR) > 20.0 * goertzel(a, 3000.0, kImmSR),
          "buffer: 1 kHz content resampled to the output rate");
    rig.be->setParam(rig.buf, Param::Gain, 0.5f);
    check(std::fabs(rms(rig.render()) / r - 0.5) < 0.02, "buffer: Gain (AudioClip.gain) scales the clip");
    rig.be->setParam(rig.buf, Param::Gain, 1.0f);
    rig.be->setParam(rig.buf, Param::PlaybackRate, 2);
    const std::vector<float> b = rig.render();
    check(goertzel(b, 2000.0, kImmSR) > 20.0 * goertzel(b, 1000.0, kImmSR),
          "buffer: PlaybackRate 2 (AudioClip.pitch) doubles the pitch");
    rig.be->setParam(rig.buf, Param::PlaybackState, 2);
    check(rms(rig.render()) < 1e-6, "buffer: paused is silent");
    rig.be->setParam(rig.buf, Param::PlaybackState, 0);
    check(rms(rig.render()) < 1e-6, "buffer: stopped is silent");
  }
}

} // namespace x3d::test

#endif
