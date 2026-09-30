// sound_delay_test.cpp — the §16 Delay node (Sound component, clause 16.4.6):
// a pure delay of its input by delayTime seconds, clamped to [0, maxDelayTime].
//   (a) RECORDING tier: a Delay{AudioDestination <- Delay <- OscillatorSource}
//       scene -> SoundSystem builds a NodeKind::Delay node, wires it, and reads
//       delayTime/maxDelayTime/enabled; update() pushes them as setParam.
//   (b) DSP tier (BuiltinDspBackend): an impulse into Delay comes out one
//       impulse, delayTime later, at the right sample.
//   (c) CLAMP: delayTime > maxDelayTime clamps to maxDelayTime.
//   (d) enabled=false passes the input through unchanged.

#include "AudioBackend.hpp"
#include "RecordingBackend.hpp"
#include "SoundSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/Delay.hpp"
#include "x3d/nodes/OscillatorSource.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
      ++g_failures;                                                             \
    }                                                                           \
  } while (0)

// Build Destination <- Delay <- Buffer(impulse) directly on the backend and
// render it. The impulse is a 1.0 at samples[0] of a long buffer (no wrap over
// the render window).
static void renderImpulse(const std::shared_ptr<BuiltinDspBackend> &dsp,
                          float delayTime, float maxDelayTime, bool enabled,
                          int frames, float sr, std::vector<float> &out) {
  NodeParams bp;
  bp.samples.assign(1024, 0.0f);
  bp.samples[0] = 1.0f;
  bp.sampleRate = sr;
  bp.gain = 1.0f;
  NodeHandle hBuf = dsp->createNode(NodeKind::Buffer, bp);
  dsp->setParam(hBuf, Param::PlaybackState, 1.0f);  // start playing

  NodeParams dp;
  dp.delayTime = delayTime;
  dp.maxDelayTime = maxDelayTime;
  dp.enabled = enabled;
  NodeHandle hDelay = dsp->createNode(NodeKind::Delay, dp);

  NodeParams destP;
  NodeHandle hDest = dsp->createNode(NodeKind::Destination, destP);

  dsp->connect(hDelay, hBuf);   // buffer feeds INTO delay
  dsp->connect(hDest, hDelay);  // delay feeds INTO destination
  dsp->render(hDest, frames, sr, out);
}

// Index of the largest |sample| in `buf`.
static std::size_t peakIndex(const std::vector<float> &buf) {
  std::size_t best = 0;
  for (std::size_t i = 1; i < buf.size(); ++i)
    if (std::fabs(buf[i]) > std::fabs(buf[best])) best = i;
  return best;
}

int main() {
  constexpr float kSR = 48000.0f;

  // ── (a) RECORDING tier: SoundSystem builds + wires a Delay node.
  {
    auto osc = std::make_shared<OscillatorSource>();
    auto delay = std::make_shared<Delay>();
    delay->setDelayTime(0.25);
    delay->setMaxDelayTime(0.5);
    delay->setChildren(MFNode{std::static_pointer_cast<X3DNode>(osc)});
    auto dest = std::make_shared<AudioDestination>();
    dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(delay)});

    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    sys.attach(dest.get(), ctx);

    CHECK(rec->creates.size() == 3, "3 nodes created (dest/delay/osc)");
    CHECK(rec->connects.size() == 2, "2 connections (osc->delay, delay->dest)");
    CHECK(sys.nodeCount() == 3, "SoundSystem mapped 3 nodes");

    NodeHandle hDelay = 0;
    NodeParams delayP;
    bool foundDelay = false;
    for (auto &cr : rec->creates) {
      if (cr.kind == NodeKind::Delay) {
        foundDelay = true;
        hDelay = cr.handle;
        delayP = cr.params;
      }
    }
    CHECK(foundDelay, "a NodeKind::Delay node was created");
    CHECK(std::fabs(delayP.delayTime - 0.25f) < 1e-6f, "delayTime read = 0.25");
    CHECK(std::fabs(delayP.maxDelayTime - 0.5f) < 1e-6f, "maxDelayTime read = 0.5");
    CHECK(delayP.enabled, "enabled read = true");

    // update() re-reads the (possibly route-animated) fields -> setParam.
    delay->setDelayTime(0.125);
    sys.update(0.0, ctx);
    bool pushedDelay = false;
    for (auto &sp : rec->setParams)
      if (sp.node == hDelay && sp.param == Param::DelayTime) {
        pushedDelay = std::fabs(sp.value - 0.125f) < 1e-6f;
      }
    CHECK(pushedDelay, "update() pushes Param::DelayTime = 0.125");

    std::fprintf(stderr, "[recording] Delay built + wired, 3 nodes, 2 edges\n");
  }

  // ── (b) DSP tier: impulse in -> impulse out delayTime later.
  {
    const int frames = 256;
    const int delaySamples = 100;
    auto dsp = std::make_shared<BuiltinDspBackend>();
    std::vector<float> out;
    renderImpulse(dsp, static_cast<float>(delaySamples) / kSR, 0.01f, true,
                  frames, kSR, out);

    CHECK(out.size() == static_cast<std::size_t>(frames), "rendered `frames`");
    CHECK(std::fabs(out[0]) < 1e-6f, "no output before the delay elapses");
    CHECK(std::fabs(out[delaySamples]) > 0.99f, "impulse appears at the delay");
    CHECK(peakIndex(out) == static_cast<std::size_t>(delaySamples),
          "the delayed impulse is at sample delaySamples");
    // Exactly one non-zero sample (a pure delay adds no smearing). Tolerance
    // 1e-4 absorbs the float rounding of delayTime -> sample count.
    int nonzero = 0;
    for (float v : out) if (std::fabs(v) > 1e-4f) ++nonzero;
    CHECK(nonzero == 1, "impulse out is a single sample");
    std::fprintf(stderr, "[dsp] delay %d samples -> peak at %zu\n",
                 delaySamples, peakIndex(out));
  }

  // ── (c) CLAMP: delayTime > maxDelayTime clamps to maxDelayTime.
  {
    const int frames = 128;
    const int maxSamples = 48;  // maxDelayTime = 0.001 s * 48000
    auto dsp = std::make_shared<BuiltinDspBackend>();
    std::vector<float> out;
    renderImpulse(dsp, /*delayTime=*/0.5f, /*maxDelayTime=*/0.001f, true,
                  frames, kSR, out);
    CHECK(peakIndex(out) == static_cast<std::size_t>(maxSamples),
          "delayTime > maxDelayTime clamps to maxDelayTime");
    CHECK(std::fabs(out[maxSamples]) > 0.99f, "clamped impulse at maxDelayTime");
    std::fprintf(stderr, "[dsp] clamp 0.5s -> peak at %zu (max=%d)\n",
                 peakIndex(out), maxSamples);
  }

  // ── (d) enabled=false passes the input through unchanged.
  {
    const int frames = 64;
    auto dsp = std::make_shared<BuiltinDspBackend>();
    std::vector<float> out;
    renderImpulse(dsp, /*delayTime=*/100.0f / kSR, /*maxDelayTime=*/0.01f,
                  /*enabled=*/false, frames, kSR, out);
    CHECK(std::fabs(out[0] - 1.0f) < 1e-6f,
          "disabled Delay passes the impulse straight through");
    CHECK(peakIndex(out) == 0, "no delay applied while disabled");
    std::fprintf(stderr, "[dsp] enabled=false passthrough peak at %zu\n",
                 peakIndex(out));
  }

  if (g_failures == 0)
    std::fprintf(stderr, "sound_delay_test: ALL PASS\n");
  return g_failures == 0 ? 0 : 1;
}
