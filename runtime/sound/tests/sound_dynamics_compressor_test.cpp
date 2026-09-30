// sound_dynamics_compressor_test.cpp — the §16.4.9 DynamicsCompressor (SND-5),
// in two tiers following sound_system_test.cpp:
//   (a) RECORDING tier: a §16 Oscillator -> DynamicsCompressor -> AudioDestination
//       scene -> SoundSystem.attach(RecordingBackend) -> assert the compressor
//       subtree is BUILT (3 nodes, 2 child->parent connections), not skipped.
//   (b) DSP tier (BuiltinDspBackend, direct NodeParams graph — the waveform
//       seam carries Square, which SoundSystem's §16 OscillatorSource mapping
//       does not): a SQUARE wave has a constant |x| level, so the gain computer
//       settles to an exactly predictable value:
//       gain_dB = (1/ratio - 1) * (level_dB - threshold)  (hard knee, knee = 0).
//       - a level below the threshold passes ~unchanged (unity gain);
//       - a level well above the threshold settles at
//         threshold + (level - threshold)/ratio once the attack ramps in;
//       - enabled=false passes the input through unchanged.
//   (c) SoundSystem end-to-end: a sine (the §16 mapping's only waveform) through
//       the same compressor is compressed to roughly the peak-level ratio law.

#include "AudioBackend.hpp"
#include "RecordingBackend.hpp"
#include "SoundSystem.hpp"
#include "SoundTimeSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"
#include "tests/dsp_metrics.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/DynamicsCompressor.hpp"
#include "x3d/nodes/OscillatorSource.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::test::rms;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
      ++g_failures;                                                             \
    }                                                                           \
  } while (0)

constexpr float kSR = 48000.0f;
constexpr int kFrames = 4096;

// Build §16 AudioDestination <- DynamicsCompressor <- OscillatorSource.
// children = inputs. Returns the nodes so a tier can mutate/rerender.
struct CompChain {
  std::shared_ptr<OscillatorSource> osc;
  std::shared_ptr<DynamicsCompressor> comp;
  std::shared_ptr<AudioDestination> dest;
};

static CompChain buildCompChain(float oscAmp, float threshold, float knee,
                                float ratio) {
  CompChain c;
  c.osc = std::make_shared<OscillatorSource>();
  c.osc->setFrequency(440.0f);
  c.osc->setGain(oscAmp);

  c.comp = std::make_shared<DynamicsCompressor>();
  c.comp->setThreshold(threshold);
  c.comp->setKnee(knee);
  c.comp->setRatio(ratio);
  c.comp->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.osc)});

  c.dest = std::make_shared<AudioDestination>();
  c.dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.comp)});
  return c;
}

// Render Oscillator(Square) -> Compressor -> Destination directly on the
// built-in backend. The square's constant |x| makes the settled gain exact.
static std::vector<float> renderDirect(float oscAmp, float threshold,
                                       bool enabled) {
  BuiltinDspBackend dsp;
  NodeParams op;
  op.waveform = Waveform::Square;
  op.gain = oscAmp;
  const NodeHandle hOsc = dsp.createNode(NodeKind::Oscillator, op);
  NodeParams cp;
  cp.threshold = threshold;
  cp.knee = 0.0f;
  cp.ratio = 12.0f;
  cp.enabled = enabled;
  const NodeHandle hCmp = dsp.createNode(NodeKind::Compressor, cp);
  const NodeHandle hDst = dsp.createNode(NodeKind::Destination, NodeParams{});
  dsp.connect(hCmp, hOsc);
  dsp.connect(hDst, hCmp);

  std::vector<float> buf;
  dsp.render(hDst, kFrames, kSR, buf);
  return buf;
}

int main() {
  // ── (a) RECORDING tier: the compressor and its subtree are built.
  {
    CompChain c = buildCompChain(0.5f, -12.0f, 0.0f, 12.0f);
    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    sys.attach(c.dest.get(), ctx);

    CHECK(rec->creates.size() == 3, "3 nodes created (osc/compressor/dest)");
    CHECK(rec->connects.size() == 2, "2 connections (the chain edges)");
    CHECK(sys.nodeCount() == 3, "SoundSystem mapped the compressor subtree");
    std::fprintf(stderr, "[recording] %zu nodes, %zu edges\n",
                 rec->creates.size(), rec->connects.size());
  }

  // ── (b) DSP: a below-threshold signal passes ~unchanged.
  {
    // A square of amplitude 0.1 sits at -20 dB, below the -12 dB threshold:
    // the gain computer stays at unity, so the RMS is untouched.
    const std::vector<float> buf = renderDirect(0.1f, -12.0f, true);
    CHECK(buf.size() == static_cast<std::size_t>(kFrames), "rendered kFrames");
    const double r = rms(buf);
    std::fprintf(stderr, "[dsp below] RMS=%.4f (expect ~0.1)\n", r);
    CHECK(std::fabs(r - 0.1) < 0.002, "below-threshold signal passes unchanged");
  }

  // ── (b2) DSP: a steady above-threshold signal settles at the ratio law.
  {
    // Constant |x| = 0.5 (-6.02 dB) against the -12 dB threshold: after the
    // attack settles, the applied gain is (1/12 - 1) * 5.98 dB = -5.48 dB
    // (0.532 linear), i.e. the level rides at threshold + (level-thr)/ratio.
    const double expected =
        0.5 * std::pow(10.0, (1.0 / 12.0 - 1.0) *
                                  (20.0 * std::log10(0.5) + 12.0) / 20.0);
    const std::vector<float> buf = renderDirect(0.5f, -12.0f, true);
    // Skip the first 1024 samples (the attack ramp) before measuring.
    const std::vector<float> tail(buf.begin() + 1024, buf.end());
    const double r = rms(tail);
    std::fprintf(stderr, "[dsp above] tail RMS=%.4f (expect ~%.4f)\n", r,
                 expected);
    CHECK(r < 0.4 && r > 0.15, "above-threshold signal is compressed");
    CHECK(std::fabs(r - expected) < 0.03,
          "settled level matches threshold + (level - threshold)/ratio");
  }

  // ── (b3) DSP: enabled=false passes the input through unchanged.
  {
    const std::vector<float> buf = renderDirect(0.5f, -12.0f, false);
    const double r = rms(buf);
    std::fprintf(stderr, "[dsp disabled] RMS=%.4f (expect ~0.5)\n", r);
    CHECK(std::fabs(r - 0.5) < 0.005, "disabled compressor passes through");
  }

  // ── (c) SoundSystem end-to-end: a sine through the full §16 path.
  {
    CompChain c = buildCompChain(0.5f, -12.0f, 0.0f, 12.0f);
    auto dsp = std::make_shared<BuiltinDspBackend>();
    auto sys = std::make_shared<SoundSystem>(dsp);
    auto clock = std::make_shared<SoundTimeSystem>();

    Scene scene;
    scene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(c.dest));
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    sys->attach(c.dest.get(), ctx);
    clock->attach(c.osc.get(), ctx);
    clock->attach(c.comp.get(), ctx);
    ctx.addSystem(clock);
    ctx.addSystem(sys);

    // The tick path: SoundTimeSystem activates the compressor (§8.2.4), the
    // cascade delivers isActive, and SoundSystem::update pushes the fields +
    // PlaybackState. The compressor must stay audible through the lifecycle,
    // not be gated to silence.
    ctx.tick(0.0);
    ctx.tick(0.5);

    std::vector<float> buf;
    sys->render(kFrames, kSR, buf);
    // Peak-level ratio law for a 0.5 sine: -6.02 dB is 5.98 dB over the -12 dB
    // threshold -> gain 0.532 -> RMS 0.3536 * 0.532 = 0.188. The one-pole
    // detector lags the peaks slightly (the attack time constant exceeds a
    // half period), so allow a loose band; the point is compressed, not
    // silenced and not unity.
    const std::vector<float> tail(buf.begin() + 1024, buf.end());
    const double r = rms(tail);
    std::fprintf(stderr, "[dsp sys sine] tail RMS=%.4f (peak-law ~0.188)\n", r);
    CHECK(r > 0.12 && r < 0.25, "SoundSystem-built compressor compresses");
  }

  if (g_failures > 0)
    std::fprintf(stderr, "%d check(s) FAILED\n", g_failures);
  else
    std::fprintf(stderr, "all dynamics-compressor checks passed\n");
  return g_failures > 0 ? 1 : 0;
}
