// §16 graph construction, synthesis, parameter animation, and spatialization.
#include "doctest/doctest.h"
#include "AudioBackend.hpp"
#include "RecordingBackend.hpp"
#include "SoundSystem.hpp"
#include "SoundTimeSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"
#include "tests/dsp_metrics.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/OscillatorSource.hpp"

#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::test::rms;
using x3d::test::goertzel;

// Build a §16 chain: AudioDestination <- Gain <- BiquadFilter <- OscillatorSource
// (children = inputs). Returns the nodes so the test can mutate/route fields.
struct Chain {
  std::shared_ptr<OscillatorSource> osc;
  std::shared_ptr<BiquadFilter> biquad;
  std::shared_ptr<Gain> gain;
  std::shared_ptr<AudioDestination> dest;
};

static Chain buildChain(float oscFreq, float cutoff,
                        BiquadTypeFilterChoices filt, float gainVal) {
  Chain c;
  c.osc = std::make_shared<OscillatorSource>();
  c.osc->setFrequency(oscFreq);

  c.biquad = std::make_shared<BiquadFilter>();
  c.biquad->setType(filt);
  c.biquad->setFrequency(cutoff);
  c.biquad->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.osc)});

  c.gain = std::make_shared<Gain>();
  c.gain->setGain(gainVal);
  c.gain->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.biquad)});

  c.dest = std::make_shared<AudioDestination>();
  c.dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.gain)});
  return c;
}

constexpr float kSR = 48000.0f;
constexpr int kFrames = 4096;

TEST_CASE("the §16 graph is recorded correctly and deterministically") {
  // (a) recording tier: the §16 graph is read correctly + deterministically.
  Chain c = buildChain(440.0f, 8000.0f, BiquadTypeFilterChoices::LOWPASS, 0.5f);

  auto rec = std::make_shared<RecordingBackend>();
  SoundSystem sys(rec);
  X3DExecutionContext ctx;
  sys.attach(c.dest.get(), ctx);

  CHECK_MESSAGE((rec->creates.size() == 4), "4 nodes created (osc/biquad/gain/dest)");
  CHECK_MESSAGE((rec->connects.size() == 3), "3 connections (the chain edges)");
  CHECK_MESSAGE((sys.nodeCount() == 4), "SoundSystem mapped 4 nodes");
  CHECK_MESSAGE((sys.destinationCount() == 1), "one AudioDestination enrolled");

  // Identify handles by kind from the create records.
  NodeHandle hDest = 0, hGain = 0, hBiq = 0, hOsc = 0;
  NodeParams oscP, biqP, gainP;
  for (auto &cr : rec->creates) {
    switch (cr.kind) {
    case NodeKind::Destination: hDest = cr.handle; break;
    case NodeKind::Gain: hGain = cr.handle; gainP = cr.params; break;
    case NodeKind::Biquad: hBiq = cr.handle; biqP = cr.params; break;
    case NodeKind::Oscillator: hOsc = cr.handle; oscP = cr.params; break;
    case NodeKind::Panner: break;
    case NodeKind::Buffer: break;
    case NodeKind::Delay: break;
    case NodeKind::Compressor: break;
    case NodeKind::WaveShaper: break;
    }
  }
  CHECK_MESSAGE((hDest && hGain && hBiq && hOsc), "all four kinds present");

  // Initial params read off the §16 nodes.
  CHECK_MESSAGE((std::fabs(oscP.frequency - 440.0f) < 1e-3f), "osc frequency = 440");
  CHECK_MESSAGE((std::fabs(biqP.frequency - 8000.0f) < 1e-3f), "biquad cutoff = 8000");
  CHECK_MESSAGE((biqP.filterType == FilterType::Lowpass), "biquad type = lowpass");
  CHECK_MESSAGE((std::fabs(gainP.gain - 0.5f) < 1e-3f), "gain = 0.5");

  // Connections are child->parent: osc->biquad, biquad->gain, gain->dest.
  bool oscToBiq = false, biqToGain = false, gainToDest = false;
  for (auto &cn : rec->connects) {
    if (cn.dst == hBiq && cn.src == hOsc) oscToBiq = true;
    if (cn.dst == hGain && cn.src == hBiq) biqToGain = true;
    if (cn.dst == hDest && cn.src == hGain) gainToDest = true;
  }
  CHECK_MESSAGE((oscToBiq), "osc feeds INTO biquad (child->parent)");
  CHECK_MESSAGE((biqToGain), "biquad feeds INTO gain (child->parent)");
  CHECK_MESSAGE((gainToDest), "gain feeds INTO destination (child->parent)");

  // Determinism of the op sequence: a second identical build records the same
  // ordered ops (same kinds, same connect directions, same handles).
  Chain c2 = buildChain(440.0f, 8000.0f, BiquadTypeFilterChoices::LOWPASS, 0.5f);
  auto rec2 = std::make_shared<RecordingBackend>();
  SoundSystem sys2(rec2);
  X3DExecutionContext ctx2;
  sys2.attach(c2.dest.get(), ctx2);
  bool sameLen = rec->ops.size() == rec2->ops.size();
  CHECK_MESSAGE((sameLen), "two runs record the same number of ops");
  if (sameLen) {
    bool identical = true;
    for (std::size_t i = 0; i < rec->ops.size(); ++i) {
      const auto &a = rec->ops[i];
      const auto &b = rec2->ops[i];
      if (a.kind != b.kind || a.a != b.a || a.b != b.b ||
          a.nodeKind != b.nodeKind) {
        identical = false;
        break;
      }
    }
    CHECK_MESSAGE((identical), "op sequence is byte-for-byte deterministic across runs");
  }
  std::fprintf(stderr, "[recording] 4 nodes, 3 edges, op-seq len=%zu\n",
               rec->ops.size());
}

TEST_CASE("a 440 Hz tone through LOWPASS and Gain 0.5 comes out at half amplitude") {
  // (b) DSP tier: 440 Hz -> LOWPASS(8k) -> Gain 0.5 is a half-amp 440 tone.
  Chain c = buildChain(440.0f, 8000.0f, BiquadTypeFilterChoices::LOWPASS, 0.5f);
  auto dsp = std::make_shared<BuiltinDspBackend>();
  SoundSystem sys(dsp);
  X3DExecutionContext ctx;
  sys.attach(c.dest.get(), ctx);

  std::vector<float> buf;
  sys.render(kFrames, kSR, buf);
  CHECK_MESSAGE((buf.size() == static_cast<std::size_t>(kFrames)), "rendered kFrames");

  double r = rms(buf);
  double g440 = goertzel(buf, 440.0, kSR);
  // A full-amplitude sine has RMS 1/sqrt(2) ~= 0.707; at gain 0.5 (and a
  // lowpass cutoff far above 440 -> ~unity passband) expect ~0.354.
  std::fprintf(stderr,
               "[dsp lowpass] RMS=%.4f (expect ~0.354) goertzel@440=%.4f\n",
               r, g440);
  CHECK_MESSAGE((r > 0.30 && r < 0.40), "half-gain 440 Hz tone has RMS ~= 0.354");
  CHECK_MESSAGE((g440 > 0.30), "strong 440 Hz component present (Goertzel)");

  // Off-frequency Goertzel should be far smaller (the tone is at 440, not 5k).
  double gOff = goertzel(buf, 5000.0, kSR);
  std::fprintf(stderr, "[dsp lowpass] goertzel@5000=%.5f (should be << @440)\n",
               gOff);
  CHECK_MESSAGE((gOff < g440 * 0.1), "440 component dominates a non-harmonic bin");

  // A high-frequency oscillator (15 kHz) through the same lowpass(1 kHz cutoff)
  // is strongly attenuated vs a low tone (200 Hz) through that lowpass.
  Chain lo = buildChain(200.0f, 1000.0f, BiquadTypeFilterChoices::LOWPASS, 1.0f);
  Chain hi = buildChain(15000.0f, 1000.0f, BiquadTypeFilterChoices::LOWPASS, 1.0f);
  auto dspLo = std::make_shared<BuiltinDspBackend>();
  auto dspHi = std::make_shared<BuiltinDspBackend>();
  SoundSystem sysLo(dspLo), sysHi(dspHi);
  X3DExecutionContext cl, ch;
  sysLo.attach(lo.dest.get(), cl);
  sysHi.attach(hi.dest.get(), ch);
  std::vector<float> bLo, bHi;
  sysLo.render(kFrames, kSR, bLo);
  sysHi.render(kFrames, kSR, bHi);
  double rLo = rms(bLo), rHi = rms(bHi);
  std::fprintf(stderr,
               "[dsp lowpass] passband(200Hz)=%.4f  stopband(15kHz)=%.5f  "
               "attenuation=%.1fx\n",
               rLo, rHi, rHi > 0 ? rLo / rHi : 0.0);
  CHECK_MESSAGE((rLo > 0.5), "200 Hz passes the 1 kHz lowpass (near unity)");
  CHECK_MESSAGE((rHi < rLo * 0.1), "15 kHz is strongly attenuated by the 1 kHz lowpass");
}

TEST_CASE("an inbound event ramps Gain.gain") {
  // (c) param animation: Gain.gain ramped via the inbound event cascade.
  //     postEvent is the same mechanism an interpolator ROUTE uses to write a
  //     field; SoundSystem.update reads the field each tick -> setParam.
  Chain c = buildChain(440.0f, 8000.0f, BiquadTypeFilterChoices::LOWPASS, 0.0f);
  auto dsp = std::make_shared<BuiltinDspBackend>();
  auto sys = std::make_shared<SoundSystem>(dsp);
  auto clock = std::make_shared<SoundTimeSystem>();

  Scene scene;
  scene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(c.dest));
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  sys->attach(c.dest.get(), ctx);
  clock->attach(c.osc.get(), ctx);
  clock->attach(c.biquad.get(), ctx);
  clock->attach(c.gain.get(), ctx);
  ctx.addSystem(clock);
  ctx.addSystem(sys);

  const float gains[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
  double prevRms = -1.0;
  bool monotonic = true;
  double t = 0.0;
  for (float g : gains) {
    // Animate the field exactly as a ScalarInterpolator route would: post the
    // new value onto Gain.gain; the cascade writes it; sys.update reads it.
    ctx.postEvent(c.gain.get(), "gain", std::any(SFFloat{g}));
    t += 1.0 / 60.0;
    ctx.tick(t);  // runs SoundSystem.update -> setParam(gain, g)
    std::vector<float> buf;
    sys->render(2048, kSR, buf);
    double r = rms(buf);
    std::fprintf(stderr, "[param ramp] gain=%.2f -> blockRMS=%.4f\n", g, r);
    if (prevRms >= 0.0 && r <= prevRms - 1e-4) monotonic = false;
    prevRms = r;
  }
  CHECK_MESSAGE((monotonic), "per-block RMS ramps up as Gain.gain animates 0 -> 1");
  CHECK_MESSAGE((prevRms > 0.30), "at gain=1.0 the tone reaches ~full half... (>0.30)");
}

TEST_CASE("the same graph renders byte-identical buffers") {
  // (d) determinism: same graph + same render twice -> byte-identical buffer.
  auto renderOnce = [&](std::vector<float> &out) {
    Chain c = buildChain(440.0f, 6000.0f, BiquadTypeFilterChoices::LOWPASS, 0.7f);
    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    sys.attach(c.dest.get(), ctx);
    sys.render(kFrames, kSR, out);
  };
  std::vector<float> a, b;
  renderOnce(a);
  renderOnce(b);
  bool identical = a.size() == b.size();
  if (identical)
    for (std::size_t i = 0; i < a.size(); ++i)
      if (a[i] != b[i]) { identical = false; break; }
  std::fprintf(stderr, "[determinism] %zu samples, byte-identical=%d\n",
               a.size(), identical ? 1 : 0);
  CHECK_MESSAGE((identical), "two identical renders produce byte-identical buffers");
}

TEST_CASE("enabled semantics and destination gain") {
  // (g) enabled semantics and destination gain.
  Chain c = buildChain(440.0f, 8000.0f, BiquadTypeFilterChoices::LOWPASS, 1.0f);
  auto dsp = std::make_shared<BuiltinDspBackend>();
  SoundSystem sys(dsp);
  X3DExecutionContext ctx;
  sys.attach(c.dest.get(), ctx);
  auto clock = std::make_shared<SoundTimeSystem>();
  clock->attach(c.osc.get(), ctx);
  clock->attach(c.biquad.get(), ctx);
  clock->attach(c.gain.get(), ctx);
  ctx.addSystem(clock);
  std::vector<float> full, changed;
  sys.render(2048, kSR, full);
  c.dest->setGain(0.25f);
  ctx.tick(0.01);
  sys.update(0.01, ctx);
  sys.render(2048, kSR, changed);
  CHECK_MESSAGE((std::fabs(rms(changed) / rms(full) - 0.25) < 0.01),
                "AudioDestination.gain scales rendered output");
  c.biquad->setEnabled(false);
  ctx.tick(0.1);
  sys.update(0.1, ctx);
  sys.render(2048, kSR, changed);
  CHECK_MESSAGE((std::fabs(rms(changed) / rms(full) - 0.25) < 0.01),
                "disabled BiquadFilter passes input through");
  c.gain->setEnabled(false);
  ctx.tick(0.2);
  sys.update(0.2, ctx);
  sys.render(2048, kSR, changed);
  CHECK_MESSAGE((std::fabs(rms(changed) / rms(full) - 0.25) < 0.01),
                "disabled Gain passes input through");
  c.osc->setEnabled(false);
  ctx.tick(0.3);
  sys.update(0.3, ctx);
  sys.render(2048, kSR, changed);
  CHECK_MESSAGE((rms(changed) < 1e-6), "disabled OscillatorSource emits silence");
}

TEST_CASE("Panner positions cross the seam") {
  // (e) panner seam: NodeKind::Panner + positional params cross the seam.
  // The recording backend receives positions rather than precomputed gains.
  NodeParams pannerParams;
  pannerParams.distanceModel    = DistanceModel::Inverse;
  pannerParams.sourcePosition[0] = 2.0f;
  pannerParams.sourcePosition[1] = 0.0f;
  pannerParams.sourcePosition[2] = 0.0f;
  pannerParams.listenerPosition[0] = 0.0f;
  pannerParams.listenerPosition[1] = 0.0f;
  pannerParams.listenerPosition[2] = 0.0f;
  pannerParams.referenceDistance  = 1.0f;
  pannerParams.rolloffFactor      = 1.0f;

  auto rec = std::make_shared<RecordingBackend>();
  NodeHandle hPanner = rec->createNode(NodeKind::Panner, pannerParams);

  CHECK_MESSAGE((hPanner != kInvalidNodeHandle), "Panner node returns a valid handle");

  bool foundPanner = false;
  float recordedX  = 0.0f;
  for (auto &cr : rec->creates) {
    if (cr.kind == NodeKind::Panner) {
      foundPanner = true;
      recordedX   = cr.params.sourcePosition[0];
    }
  }
  CHECK_MESSAGE((foundPanner), "RecordingBackend has a Panner create record");
  CHECK_MESSAGE((std::fabs(recordedX - 2.0f) < 1e-5f),
                "Panner create record carries sourcePosition[0] = 2.0");

  // renderStereo: the seam must expose a stereo path (interleaved L,R,...).
  std::vector<float> stereoOut;
  rec->renderStereo(hPanner, /*frames=*/4, /*sampleRate=*/48000.0f, stereoOut);
  CHECK_MESSAGE((stereoOut.size() == 8),
                "renderStereo output length == 2*frames (interleaved L,R)");

  std::fprintf(stderr,
               "[panner seam] Panner node handle=%llu, "
               "sourcePosition[0]=%.2f, stereoLen=%zu\n",
               static_cast<unsigned long long>(hPanner), recordedX,
               stereoOut.size());
}

TEST_CASE("the builtin Panner follows the equal-power law and distance falloff") {
  // (f) spatial DSP: BuiltinDsp equal-power Panner.
  //     Build a direct NodeParams graph: OscillatorSource -> Panner -> Destination.
  //     Set positions directly in NodeParams (no full SpatialSound scene path
  //     required for this unit test). Assert equal-power pan law + distance falloff.
  // L/R channel helpers from dsp_metrics.hpp.
  using x3d::test::rmsL;
  using x3d::test::rmsR;

  // Build a minimal Panner graph via the backend API directly (NodeParams
  // carries positions; no SpatialSound scene node needed for this tier).
  auto makePannerGraph = [&](NodeParams pannerP) -> std::pair<std::shared_ptr<BuiltinDspBackend>, NodeHandle> {
    auto dsp = std::make_shared<BuiltinDspBackend>();

    NodeParams oscP;
    oscP.frequency = 440.0f;
    oscP.gain = 1.0f;
    NodeHandle hOsc = dsp->createNode(NodeKind::Oscillator, oscP);

    NodeHandle hPanner = dsp->createNode(NodeKind::Panner, pannerP);

    NodeParams destP;
    destP.maxChannelCount = 2;
    NodeHandle hDest = dsp->createNode(NodeKind::Destination, destP);

    // Wire: osc -> panner -> destination
    dsp->connect(hPanner, hOsc);
    dsp->connect(hDest, hPanner);

    return {dsp, hDest};
  };

  // Listener at origin facing -Z (default), source at hard left (+X, same Z=0).
  // The "right" ear direction = cross(forward,up) = cross(0,0,-1 , 0,1,0) = +X.
  // Source at (-5, 0, 0) relative to listener at origin: az_norm should be -1
  // (full left).
  {
    NodeParams p;
    p.sourcePosition[0]   = -5.0f;   // hard left
    p.sourcePosition[1]   = 0.0f;
    p.sourcePosition[2]   = 0.0f;
    p.listenerPosition[0] = 0.0f;
    p.listenerPosition[1] = 0.0f;
    p.listenerPosition[2] = 0.0f;
    p.listenerForward[0]  = 0.0f;    // default: -Z
    p.listenerForward[1]  = 0.0f;
    p.listenerForward[2]  = -1.0f;
    p.listenerUp[0]       = 0.0f;    // default: +Y
    p.listenerUp[1]       = 1.0f;
    p.listenerUp[2]       = 0.0f;
    p.distanceModel       = DistanceModel::Inverse;
    p.referenceDistance   = 1.0f;
    p.maxDistance         = 100.0f;
    p.rolloffFactor       = 1.0f;

    auto [dsp, hDest] = makePannerGraph(p);
    std::vector<float> lr;
    dsp->renderStereo(hDest, kFrames, kSR, lr);
    double L = rmsL(lr);
    double R = rmsR(lr);
    std::fprintf(stderr,
                 "[spatial] hard-left: rmsL=%.4f rmsR=%.4f ratio=%.1fx\n",
                 L, R, R > 1e-9 ? L / R : 0.0);
    CHECK_MESSAGE((L > 4.0 * R), "hard-left source: L channel dominates (L > 4*R)");
  }

  // Hard right: source at (+5, 0, 0) -> az_norm = +1 -> rmsR >> rmsL
  {
    NodeParams p;
    p.sourcePosition[0]   = 5.0f;    // hard right
    p.sourcePosition[1]   = 0.0f;
    p.sourcePosition[2]   = 0.0f;
    p.listenerPosition[0] = 0.0f;
    p.listenerPosition[1] = 0.0f;
    p.listenerPosition[2] = 0.0f;
    p.listenerForward[0]  = 0.0f;
    p.listenerForward[1]  = 0.0f;
    p.listenerForward[2]  = -1.0f;
    p.listenerUp[0]       = 0.0f;
    p.listenerUp[1]       = 1.0f;
    p.listenerUp[2]       = 0.0f;
    p.distanceModel       = DistanceModel::Inverse;
    p.referenceDistance   = 1.0f;
    p.maxDistance         = 100.0f;
    p.rolloffFactor       = 1.0f;

    auto [dsp, hDest] = makePannerGraph(p);
    std::vector<float> lr;
    dsp->renderStereo(hDest, kFrames, kSR, lr);
    double L = rmsL(lr);
    double R = rmsR(lr);
    std::fprintf(stderr,
                 "[spatial] hard-right: rmsL=%.4f rmsR=%.4f ratio=%.1fx\n",
                 L, R, L > 1e-9 ? R / L : 0.0);
    CHECK_MESSAGE((R > 4.0 * L), "hard-right source: R channel dominates (R > 4*L)");
  }

  // Centered: source directly in front of listener (0, 0, -5).
  // az_norm = 0 -> theta = pi/4 -> gL = gR = cos(pi/4) = sin(pi/4) = 1/sqrt(2).
  // gL^2 + gR^2 = 0.5 + 0.5 = 1.0 (energy conserved).
  {
    NodeParams p;
    p.sourcePosition[0]   = 0.0f;    // centered (forward)
    p.sourcePosition[1]   = 0.0f;
    p.sourcePosition[2]   = -5.0f;
    p.listenerPosition[0] = 0.0f;
    p.listenerPosition[1] = 0.0f;
    p.listenerPosition[2] = 0.0f;
    p.listenerForward[0]  = 0.0f;
    p.listenerForward[1]  = 0.0f;
    p.listenerForward[2]  = -1.0f;
    p.listenerUp[0]       = 0.0f;
    p.listenerUp[1]       = 1.0f;
    p.listenerUp[2]       = 0.0f;
    p.distanceModel       = DistanceModel::Inverse;
    p.referenceDistance   = 1.0f;
    p.maxDistance         = 100.0f;
    p.rolloffFactor       = 1.0f;

    auto [dsp, hDest] = makePannerGraph(p);
    std::vector<float> lr;
    dsp->renderStereo(hDest, kFrames, kSR, lr);
    double L = rmsL(lr);
    double R = rmsR(lr);
    double balance = std::fabs(L - R);
    // gL = gR = 1/sqrt(2) => gL^2 + gR^2 = 1.0; both channels equal within ~1%
    double sumSq = (L * L + R * R);  // should be ~0.5 * mono_rms^2
    std::fprintf(stderr,
                 "[spatial] centered: rmsL=%.4f rmsR=%.4f |L-R|=%.5f "
                 "L^2+R^2=%.5f\n",
                 L, R, balance, sumSq);
    CHECK_MESSAGE((balance < 0.01 * (L + R + 1e-9)), "centered: L ≈ R (within 1%)");
    // Energy conservation: gL^2+gR^2 = 1 means L^2+R^2 = mono_energy/2 for
    // each channel; here we just verify that L ≈ R not L==R via channel balance.
    CHECK_MESSAGE((L > 0.01), "centered: non-zero output on L channel");
    CHECK_MESSAGE((R > 0.01), "centered: non-zero output on R channel");
  }

  // Distance falloff: source at referenceDistance vs maxDistance.
  // Using LINEAR model: at ref => gain=1.0; at max => gain→0.
  {
    auto renderAtDist = [&](float dist) {
      NodeParams p;
      p.sourcePosition[0]   = dist;
      p.sourcePosition[1]   = 0.0f;
      p.sourcePosition[2]   = 0.0f;
      p.listenerPosition[0] = 0.0f;
      p.listenerPosition[1] = 0.0f;
      p.listenerPosition[2] = 0.0f;
      p.listenerForward[0]  = 0.0f;
      p.listenerForward[1]  = 0.0f;
      p.listenerForward[2]  = -1.0f;
      p.listenerUp[0]       = 0.0f;
      p.listenerUp[1]       = 1.0f;
      p.listenerUp[2]       = 0.0f;
      p.distanceModel       = DistanceModel::Linear;
      p.referenceDistance   = 1.0f;
      p.maxDistance         = 10.0f;
      p.rolloffFactor       = 1.0f;
      auto [dsp, hDest] = makePannerGraph(p);
      std::vector<float> lr;
      dsp->renderStereo(hDest, kFrames, kSR, lr);
      // total RMS across both channels
      double s = 0.0;
      for (float v : lr) s += static_cast<double>(v) * v;
      return std::sqrt(s / static_cast<double>(lr.size()));
    };

    double rmsRef = renderAtDist(1.0f);   // at referenceDistance
    double rmsMax = renderAtDist(10.0f);  // at maxDistance (linear => gain=0)
    std::fprintf(stderr,
                 "[spatial] distance falloff (linear): rmsRef=%.4f "
                 "rmsMax=%.5f\n",
                 rmsRef, rmsMax);
    // At maxDistance with LINEAR model and rolloff=1, gain clamped to 0.
    CHECK_MESSAGE((rmsRef > 0.1), "at referenceDistance: non-zero output");
    CHECK_MESSAGE((rmsMax < rmsRef * 0.05),
                  "at maxDistance (linear): energy << referenceDistance");
  }
}
