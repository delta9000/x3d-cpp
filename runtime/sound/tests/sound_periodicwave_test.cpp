// §16.4.18 harmonic synthesis, wave registration, and normalization.
// sin(t) + 0.5*cos(2*t) has peak magnitude 1.5 at t=3*pi/2; normalized samples
// at 0, T/4, T/2 are 1/3, and at 3*T/4 are -1.
// SND-9: X3D 4.0 bindings lack OscillatorSource.periodicWave; the parse test
// documents the deferred authored-wave gap.
#include "doctest/doctest.h"
#include "AudioBackend.hpp"
#include "InlineExpand.hpp"  // x3d::runtime::findField (generic-by-name reads)
#include "RecordingBackend.hpp"
#include "SoundSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"
#include "parse/X3DParse.hpp"
#include "tests/dsp_metrics.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/PeriodicWave.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::test::rms;
using x3d::test::goertzel;

TEST_CASE("PeriodicWave") {
  constexpr float kSR = 48000.0f;
  constexpr int kFrames = 4096;
  constexpr float kFreq = 100.0f;  // period = kSR/kFreq = 480 samples

  // Render one mono oscillator through a fresh BuiltinDspBackend (phase 0).
  auto renderOsc = [&](const NodeParams &p) {
    BuiltinDspBackend dsp;
    NodeHandle dest = dsp.createNode(NodeKind::Destination, NodeParams{});
    NodeHandle osc = dsp.createNode(NodeKind::Oscillator, p);
    dsp.connect(dest, osc);
    std::vector<float> out;
    dsp.render(dest, kFrames, kSR, out);
    return out;
  };
  auto near = [](double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
  };

  // (b) system: SoundSystem::setPeriodicWave registered waves
  // A §16 chain AudioDestination <- OscillatorSource. The 4.0 bindings cannot
  // read the authored periodicWave field (tier (c)), so the test registers the
  // PeriodicWave node the way an embedder bridges the gap.
  auto buildDest = [](const std::shared_ptr<OscillatorSource> &osc) {
    auto dest = std::make_shared<AudioDestination>();
    dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(osc)});
    return dest;
  };
  auto customWave = [] {
    auto w = std::make_shared<PeriodicWave>();
    w->setType(PeriodicWaveTypeChoices::CUSTOM);
    w->setOptionsReal(MFFloat{0.0f, 0.0f, 0.5f});
    w->setOptionsImag(MFFloat{0.0f, 1.0f, 0.0f});
    return w;
  };
  auto maxAbsDiff = [](const std::vector<float> &a, const std::vector<float> &b) {
    double d = 0.0;
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i)
      d = std::max(d, std::fabs(double(a[i]) - double(b[i])));
    return d;
  };

  SUBCASE("backend: a single-harmonic CUSTOM wave equals Sine") {
    // P1: single-harmonic CUSTOM (imag[1]=1) == Sine, sample-for-sample.
    NodeParams sine;
    sine.frequency = kFreq;
    sine.waveform = Waveform::Sine;

    NodeParams custom;
    custom.frequency = kFreq;
    custom.waveform = Waveform::Custom;
    custom.optionsReal = {0.0f, 0.0f};
    custom.optionsImag = {0.0f, 1.0f};

    std::vector<float> a = renderOsc(sine), b = renderOsc(custom);
    CHECK_MESSAGE((a.size() == static_cast<std::size_t>(kFrames)), "sine rendered");
    CHECK_MESSAGE((b.size() == static_cast<std::size_t>(kFrames)), "custom rendered");
    double maxDiff = 0.0;
    for (int i = 0; i < kFrames; ++i)
      maxDiff = std::max(maxDiff, std::fabs(double(a[std::size_t(i)]) -
                                             double(b[std::size_t(i)])));
    std::fprintf(stderr,
                 "[P1 single-harmonic] max|custom - sine| = %.3g (expect ~0)\n",
                 maxDiff);
    CHECK_MESSAGE((maxDiff < 1e-6), "imag=[0,1] CUSTOM wave == Sine sample-for-sample");
  }

  SUBCASE("backend: a two-harmonic CUSTOM wave has the analytic values") {
    // P2: two-harmonic CUSTOM — analytic values, normalization, period.
    NodeParams custom;
    custom.frequency = kFreq;
    custom.waveform = Waveform::Custom;
    custom.optionsReal = {0.0f, 0.0f, 0.5f};  // DC ignored; 0.5*cos(2t)
    custom.optionsImag = {0.0f, 1.0f, 0.0f};  // sin(t)

    std::vector<float> b = renderOsc(custom);
    CHECK_MESSAGE((b.size() == static_cast<std::size_t>(kFrames)), "two-harmonic rendered");
    // x(t) = sin(t) + 0.5 cos(2t); normalized by the analytic peak 1.5.
    const int period = static_cast<int>(kSR / kFreq);  // 480 samples
    const std::size_t q = static_cast<std::size_t>(period) / 4;
    std::fprintf(stderr,
                 "[P2 two-harmonic] x[0]=%.6f x[T/4]=%.6f x[T/2]=%.6f "
                 "x[3T/4]=%.6f (expect 1/3, 1/3, 1/3, -1)\n",
                 b[0], b[q], b[2 * q], b[3 * q]);
    CHECK_MESSAGE((near(b[0], 1.0 / 3.0, 1e-6)), ("x(0) = (0+0.5)/1.5 = 1/3"));
    CHECK_MESSAGE((near(b[q], 1.0 / 3.0, 1e-6)), "x(T/4) = (1-0.5)/1.5 = 1/3");
    CHECK_MESSAGE((near(b[2 * q], 1.0 / 3.0, 1e-6)), ("x(T/2) = (0+0.5)/1.5 = 1/3"));
    CHECK_MESSAGE((near(b[3 * q], -1.0, 1e-6)), "x(3T/4) = (-1-0.5)/1.5 = -1");

    double peak = 0.0;
    for (float s : b) peak = std::max(peak, std::fabs(double(s)));
    std::fprintf(stderr, "[P2 two-harmonic] peak = %.6f (expect 1)\n", peak);
    CHECK_MESSAGE((near(peak, 1.0, 1e-6)), "normalized: peak absolute value 1");

    double periodDiff = 0.0;
    for (int i = 0; i + period < kFrames; ++i)
      periodDiff = std::max(
          periodDiff, std::fabs(double(b[std::size_t(i)]) -
                                double(b[std::size_t(i + period)])));
    std::fprintf(stderr,
                 "[P2 two-harmonic] max|x[i] - x[i+%d]| = %.3g (expect ~0)\n",
                 period, periodDiff);
    CHECK_MESSAGE((periodDiff < 1e-5), "wave repeats with period SR/f");
  }

  SUBCASE("backend: no custom wave keeps the Sine default") {
    // P3: no custom wave — the Sine default is unchanged.
    NodeParams p;  // waveform defaults to Sine
    p.frequency = kFreq;
    CHECK_MESSAGE((p.waveform == Waveform::Sine), "NodeParams.waveform defaults to Sine");
    std::vector<float> b = renderOsc(p);
    double r = rms(b);
    double g = goertzel(b, double(kFreq), kSR);
    std::fprintf(stderr, "[P3 unset] RMS=%.4f (sine ~0.7071) goertzel=%.4f\n",
                 r, g);
    CHECK_MESSAGE((r > 0.68 && r < 0.73), "default oscillator is a unit sine");
    CHECK_MESSAGE((g > 0.6), "strong fundamental present");
  }

  SUBCASE("system: a registered CUSTOM wave renders the harmonic sum") {
    // P4: a registered CUSTOM wave renders the harmonic sum (P2's values).
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);
    auto wave = customWave();

    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    sys.setPeriodicWave(osc.get(), wave.get());
    sys.attach(buildDest(osc).get(), ctx);
    std::vector<float> buf;
    sys.render(kFrames, kSR, buf);

    const std::size_t q = static_cast<std::size_t>(kSR / kFreq) / 4;
    std::fprintf(stderr,
                 "[P4 system custom] x[0]=%.6f x[3T/4]=%.6f (expect 1/3, -1)\n",
                 buf[0], buf[3 * q]);
    CHECK_MESSAGE((near(buf[0], 1.0 / 3.0, 1e-6)), "registered CUSTOM wave: x(0) = 1/3");
    CHECK_MESSAGE((near(buf[3 * q], -1.0, 1e-6)), "registered CUSTOM wave: x(3T/4) = -1");
  }

  SUBCASE("system: SAWTOOTH maps onto the seam sawtooth") {
    // P5: the four standard types map onto the Waveform enum — a SAWTOOTH wave
    // renders sample-for-sample like the seam's own sawtooth.
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);
    auto wave = std::make_shared<PeriodicWave>();
    wave->setType(PeriodicWaveTypeChoices::SAWTOOTH);

    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    sys.setPeriodicWave(osc.get(), wave.get());
    sys.attach(buildDest(osc).get(), ctx);
    std::vector<float> buf;
    sys.render(kFrames, kSR, buf);

    NodeParams direct;
    direct.frequency = kFreq;
    direct.waveform = Waveform::Sawtooth;
    const double d = maxAbsDiff(buf, renderOsc(direct));
    std::fprintf(stderr,
                 "[P5 system sawtooth] max|system - seam enum| = %.3g (expect ~0)\n",
                 d);
    CHECK_MESSAGE((d < 1e-6), "type=SAWTOOTH maps onto the seam's sawtooth");
  }

  SUBCASE("system: no registered wave renders sine") {
    // P6: no registered wave -> sine (the §16 default, unchanged).
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);

    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    sys.attach(buildDest(osc).get(), ctx);
    std::vector<float> buf;
    sys.render(kFrames, kSR, buf);

    NodeParams direct;
    direct.frequency = kFreq;
    direct.waveform = Waveform::Sine;
    const double d = maxAbsDiff(buf, renderOsc(direct));
    std::fprintf(stderr, "[P6 unset] max|system - sine| = %.3g (expect ~0)\n", d);
    CHECK_MESSAGE((d < 1e-6), "oscillator with NO registered wave stays sine");
  }

  SUBCASE("system: the wave rides the seam, and an empty CUSTOM falls back to sine") {
    // P7: the wave rides the seam in the node's params (recording tier) — and a
    // CUSTOM wave with no harmonic terms falls back to sine.
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);
    auto emptyOsc = std::make_shared<OscillatorSource>();
    emptyOsc->setFrequency(kFreq);
    auto dest = std::make_shared<AudioDestination>();
    dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(osc),
                             std::static_pointer_cast<X3DNode>(emptyOsc)});

    auto wave = customWave();
    auto emptyWave = std::make_shared<PeriodicWave>();
    emptyWave->setType(PeriodicWaveTypeChoices::CUSTOM);  // no options

    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    sys.setPeriodicWave(osc.get(), wave.get());
    sys.setPeriodicWave(emptyOsc.get(), emptyWave.get());
    sys.attach(dest.get(), ctx);

    // Creates follow the children recursion: osc first, then emptyOsc.
    const NodeParams *customParams = nullptr, *emptyParams = nullptr;
    int seen = 0;
    for (const auto &cr : rec->creates) {
      if (cr.kind != NodeKind::Oscillator) continue;
      if (seen++ == 0) customParams = &cr.params;
      else emptyParams = &cr.params;
    }
    CHECK_MESSAGE((customParams && customParams->waveform == Waveform::Custom),
                  "CUSTOM wave rides the seam as Waveform::Custom");
    CHECK_MESSAGE((customParams && customParams->optionsReal.size() == 3 &&
              customParams->optionsImag.size() == 3 &&
              customParams->optionsReal[2] == 0.5f &&
              customParams->optionsImag[1] == 1.0f), "optionsReal/optionsImag harmonic terms crossed the seam");
    CHECK_MESSAGE((emptyParams && emptyParams->waveform == Waveform::Sine),
                  "CUSTOM wave with no harmonic terms falls back to sine");
  }

  SUBCASE("parse: the 4.1 field is unreadable on the 4.0 bindings") {
    // (c) parse path: the 4.1 field is unreadable on the 4.0 bindings
    const char *xml =
        "<?xml version='1.0' encoding='UTF-8'?>"
        "<X3D profile='Immersive' version='4.0'>"
        "<Scene>"
        "<OscillatorSource DEF='TONE' frequency='220'>"
        "<PeriodicWave containerField='periodicWave' type='CUSTOM' "
        "optionsReal='0 0' optionsImag='0 1'/>"
        "</OscillatorSource>"
        "</Scene></X3D>";
    X3DDocument doc = x3d::codec::parseDocument(xml);

    OscillatorSource *osc = nullptr;
    for (const auto &root : doc.scene.rootNodes)
      if ((osc = dynamic_cast<OscillatorSource *>(root.get()))) break;
    CHECK_MESSAGE((osc != nullptr), "OscillatorSource parsed from the scene");
    if (osc) {
      // SND-9: generic-by-name field reads cannot reach the authored wave —
      // the 4.0 field table has no periodicWave entry.
      CHECK_MESSAGE((findField(*osc, "periodicWave") == nullptr),
                    "OscillatorSource has NO periodicWave field (X3D 4.0 object model)");
      // The parser's best-effort fallback parks the child in the IS field,
      // where Proto field connections live — unreadable as periodicWave.
      auto *wave = dynamic_cast<PeriodicWave *>(osc->getIS().get());
      std::fprintf(stderr,
                   "[parse] periodicWave field=%s; fallback parked node: %s\n",
                   findField(*osc, "periodicWave") ? "present" : "absent",
                   wave ? "PeriodicWave (in IS)" : "none");
      CHECK_MESSAGE((wave != nullptr && wave->getOptionsImag().size() == 2), "authored PeriodicWave lands in IS (parser fallback), not a "
            "readable periodicWave field");
    }
  }
}
