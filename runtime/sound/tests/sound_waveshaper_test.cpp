// §16.4.21 WaveShaper: interpolation, clamping, and disabled passthrough.
#include "doctest/doctest.h"
#include "AudioBackend.hpp"
#include "RecordingBackend.hpp"
#include "SoundSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/WaveShaper.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

// Render `src` (a Buffer played 1:1) through a WaveShaper with the given curve
// on a fresh backend, and return the destination's output.
static std::vector<float> shape(const std::vector<float> &curve, float gain,
                                bool enabled, const std::vector<float> &src) {
  constexpr float kSR = 48000.0f;
  auto dsp = std::make_shared<BuiltinDspBackend>();

  NodeParams bp;
  bp.samples = src;
  bp.sampleRate = kSR;
  bp.gain = 1.0f;
  NodeHandle hBuf = dsp->createNode(NodeKind::Buffer, bp);
  dsp->setParam(hBuf, Param::PlaybackState, 1.0f);  // start playing

  NodeParams wp;
  wp.curve = curve;
  wp.gain = gain;
  wp.enabled = enabled;
  NodeHandle hWs = dsp->createNode(NodeKind::WaveShaper, wp);

  NodeHandle hDest = dsp->createNode(NodeKind::Destination, {});

  dsp->connect(hWs, hBuf);   // buffer feeds into the waveshaper
  dsp->connect(hDest, hWs);  // waveshaper feeds into the destination

  std::vector<float> out;
  dsp->render(hDest, static_cast<int>(src.size()), kSR, out);
  return out;
}

static bool near(float a, float b, float eps = 1e-6f) {
  return std::fabs(a - b) < eps;
}

TEST_CASE("sound_waveshaper_test") {
  // (a) recording tier: SoundSystem builds + wires a WaveShaper node.
  {
    auto osc = std::make_shared<OscillatorSource>();
    auto ws = std::make_shared<WaveShaper>();
    ws->setCurve(MFFloat{-0.5f, -0.5f, 0.5f, 0.5f});  // hard clip
    ws->setChildren(MFNode{std::static_pointer_cast<X3DNode>(osc)});
    auto dest = std::make_shared<AudioDestination>();
    dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(ws)});

    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    sys.attach(dest.get(), ctx);

    CHECK_MESSAGE((rec->creates.size() == 3), "3 nodes created (dest/waveshaper/osc)");
    CHECK_MESSAGE((rec->connects.size() == 2), "2 connections (osc->ws, ws->dest)");
    CHECK_MESSAGE((sys.nodeCount() == 3), "SoundSystem mapped 3 nodes");

    NodeHandle hWs = 0;
    NodeParams wsP;
    bool foundWs = false;
    for (auto &cr : rec->creates) {
      if (cr.kind == NodeKind::WaveShaper) {
        foundWs = true;
        hWs = cr.handle;
        wsP = cr.params;
      }
    }
    CHECK_MESSAGE((foundWs), "a NodeKind::WaveShaper node was created");
    CHECK_MESSAGE((wsP.curve.size() == 4 && near(wsP.curve[0], -0.5f) &&
              near(wsP.curve[3], 0.5f)), "curve read = the hard-clip table");
    CHECK_MESSAGE((near(wsP.gain, 1.0f)), "gain read = 1");
    CHECK_MESSAGE((wsP.enabled), "enabled read = true");

    // update() re-reads the (possibly route-animated) fields -> setParam.
    ws->setGain(0.5f);
    sys.update(0.0, ctx);
    bool pushedGain = false;
    for (auto &sp : rec->setParams)
      if (sp.node == hWs && sp.param == Param::Gain) {
        pushedGain = near(sp.value, 0.5f);
      }
    CHECK_MESSAGE((pushedGain), "update() pushes Param::Gain = 0.5");

    std::fprintf(stderr, "[recording] WaveShaper built + wired, 3 nodes, 2 edges\n");
  }

  // (b) DSP tier: identity curve [-1,1] leaves the signal unchanged.
  {
    const std::vector<float> src = {0.0f, 0.25f, -0.25f, 0.5f,  -0.5f,
                                    0.9f, -0.9f, 1.0f,   -1.0f};
    const std::vector<float> out = shape({-1.0f, 1.0f}, 1.0f, true, src);
    bool same = out.size() == src.size();
    for (std::size_t i = 0; same && i < src.size(); ++i)
      same = near(out[i], src[i]);
    CHECK_MESSAGE((same), "identity curve [-1,1] leaves the signal unchanged");
    std::fprintf(stderr, "[dsp] identity curve: signal unchanged\n");
  }

  // (c) DSP tier: a hard-clip curve clamps; in-band samples map linearly.
  {
    const std::vector<float> clip = {-0.5f, -0.5f, 0.5f, 0.5f};
    const std::vector<float> out =
        shape(clip, 1.0f, true, {0.8f, -0.8f, 0.2f, -0.2f});
    CHECK_MESSAGE((near(out[0], 0.5f)), ("hard clip: +0.8 clamps to +0.5"));
    CHECK_MESSAGE((near(out[1], -0.5f)), "hard clip: -0.8 clamps to -0.5");
    CHECK_MESSAGE((near(out[2], 0.3f)), ("hard clip: +0.2 maps to +0.3"));
    CHECK_MESSAGE((near(out[3], -0.3f)), "hard clip: -0.2 maps to -0.3");
    std::fprintf(stderr, "[dsp] hard-clip curve: clamps at the flat ends\n");
  }

  // (d) DSP tier: input outside [-1,1] clamps to the end curve values.
  {
    const std::vector<float> clip = {-0.5f, -0.5f, 0.5f, 0.5f};
    const std::vector<float> out = shape(clip, 1.0f, true, {1.5f, -1.5f, 2.0f});
    CHECK_MESSAGE((near(out[0], 0.5f)), ("x=+1.5 clamps to the last curve value"));
    CHECK_MESSAGE((near(out[1], -0.5f)), "x=-1.5 clamps to the first curve value");
    CHECK_MESSAGE((near(out[2], 0.5f)), ("x=+2.0 clamps to the last curve value"));
    std::fprintf(stderr, "[dsp] out-of-range input: clamps to end values\n");
  }

  // (e) DSP tier: an empty curve passes the signal through.
  {
    const std::vector<float> src = {0.0f, 0.3f, -0.7f, 1.0f, -1.0f};
    const std::vector<float> out = shape({}, 1.0f, true, src);
    bool same = out.size() == src.size();
    for (std::size_t i = 0; same && i < src.size(); ++i)
      same = near(out[i], src[i]);
    CHECK_MESSAGE((same), "empty curve passes the signal through");
    std::fprintf(stderr, "[dsp] empty curve: pass-through\n");
  }

  // (f) DSP tier: enabled=false passes the signal through.
  {
    const std::vector<float> src = {0.0f, 0.3f, -0.7f, 0.8f};
    const std::vector<float> out =
        shape({-0.5f, -0.5f, 0.5f, 0.5f}, 1.0f, false, src);
    bool same = out.size() == src.size();
    for (std::size_t i = 0; same && i < src.size(); ++i)
      same = near(out[i], src[i]);
    CHECK_MESSAGE((same), "enabled=false passes the signal through");
    std::fprintf(stderr, "[dsp] enabled=false: pass-through\n");
  }

  // (g) DSP tier: the processing-node gain multiplies the shaped signal.
  {
    const std::vector<float> src = {0.5f, -0.5f};
    const std::vector<float> out = shape({-1.0f, 1.0f}, 0.5f, true, src);
    CHECK_MESSAGE((near(out[0], 0.25f) && near(out[1], -0.25f)), "gain scales the curve output");
    std::fprintf(stderr, "[dsp] gain=0.5 scales the shaped signal\n");
  }

  // (h) DSP tier: a single-value curve holds every sample at that value.
  {
    const std::vector<float> out = shape({0.25f}, 1.0f, true, {0.0f, 0.7f, -0.9f});
    CHECK_MESSAGE((near(out[0], 0.25f) && near(out[1], 0.25f) && near(out[2], 0.25f)),
                  "single-value curve is constant");
    std::fprintf(stderr, "[dsp] single-value curve: constant\n");
  }
}
