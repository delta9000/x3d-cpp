// §16.4.18 harmonic synthesis, wave selection, and normalization.
// sin(t) + 0.5*cos(2*t) has peak magnitude 1.5 at t=3*pi/2; normalized samples
// at 0, T/4, T/2 are 1/3, and at 3*T/4 are -1.
// SND-9: the wave is OscillatorSource.periodicWave (X3D 4.1), read at attach().
#include "doctest/doctest.h"
#include "AudioBackend.hpp"
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

  // (b) system: the oscillator's periodicWave field shapes its waveform.
  // A §16 chain AudioDestination <- OscillatorSource.
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

  SUBCASE("system: an authored CUSTOM wave renders the harmonic sum") {
    // P4: an authored CUSTOM wave renders the harmonic sum (P2's values).
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);
    auto wave = customWave();

    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    osc->setPeriodicWave(wave);
    auto dest = buildDest(osc);
    sys.attach(dest.get(), ctx);
    std::vector<float> buf;
    sys.render(kFrames, kSR, buf);

    const std::size_t q = static_cast<std::size_t>(kSR / kFreq) / 4;
    std::fprintf(stderr,
                 "[P4 system custom] x[0]=%.6f x[3T/4]=%.6f (expect 1/3, -1)\n",
                 buf[0], buf[3 * q]);
    CHECK_MESSAGE((near(buf[0], 1.0 / 3.0, 1e-6)), "authored CUSTOM wave: x(0) = 1/3");
    CHECK_MESSAGE((near(buf[3 * q], -1.0, 1e-6)), "authored CUSTOM wave: x(3T/4) = -1");
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
    osc->setPeriodicWave(wave);
    auto dest = buildDest(osc);
    sys.attach(dest.get(), ctx);
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

  SUBCASE("system: no authored wave renders sine") {
    // P6: no authored wave -> sine (the §16 default, unchanged).
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);

    auto dsp = std::make_shared<BuiltinDspBackend>();
    SoundSystem sys(dsp);
    X3DExecutionContext ctx;
    auto dest = buildDest(osc);
    sys.attach(dest.get(), ctx);
    std::vector<float> buf;
    sys.render(kFrames, kSR, buf);

    NodeParams direct;
    direct.frequency = kFreq;
    direct.waveform = Waveform::Sine;
    const double d = maxAbsDiff(buf, renderOsc(direct));
    std::fprintf(stderr, "[P6 unset] max|system - sine| = %.3g (expect ~0)\n", d);
    CHECK_MESSAGE((d < 1e-6), "oscillator with NO authored wave stays sine");
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
    osc->setPeriodicWave(wave);
    emptyOsc->setPeriodicWave(emptyWave);
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

  SUBCASE("system: changing periodicWave after attach re-sends the waveform") {
    auto osc = std::make_shared<OscillatorSource>();
    osc->setFrequency(kFreq);
    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    auto dest = buildDest(osc); // the system holds raw pointers: keep the graph alive
    sys.attach(dest.get(), ctx);
    sys.update(0, ctx);
    CHECK(rec->waveforms.empty()); // unchanged: nothing re-sent

    auto wave = std::make_shared<PeriodicWave>();
    wave->setType(PeriodicWaveTypeChoices::SQUARE);
    osc->setPeriodicWave(wave);
    sys.update(0.1, ctx);
    REQUIRE(rec->waveforms.size() == 1);
    CHECK(rec->waveforms[0].params.waveform == Waveform::Square);

    wave->setType(PeriodicWaveTypeChoices::CUSTOM);
    wave->setOptionsReal({0, 0});
    wave->setOptionsImag({0, 1});
    sys.update(0.2, ctx);
    REQUIRE(rec->waveforms.size() == 2);
    CHECK(rec->waveforms[1].params.waveform == Waveform::Custom);
    CHECK(rec->waveforms[1].params.optionsImag.size() == 2);

    osc->setPeriodicWave(nullptr);
    sys.update(0.3, ctx);
    REQUIRE(rec->waveforms.size() == 3);
    CHECK(rec->waveforms[2].params.waveform == Waveform::Sine);
    sys.update(0.4, ctx);
    CHECK(rec->waveforms.size() == 3);
  }

  SUBCASE("dsp: setWaveform switches a running oscillator") {
    BuiltinDspBackend dsp;
    NodeHandle dest = dsp.createNode(NodeKind::Destination, NodeParams{});
    NodeParams p;
    p.frequency = kFreq;
    NodeHandle osc = dsp.createNode(NodeKind::Oscillator, p);
    dsp.connect(dest, osc);
    NodeParams saw;
    saw.waveform = Waveform::Sawtooth;
    dsp.setWaveform(osc, saw);
    std::vector<float> out;
    dsp.render(dest, kFrames, kSR, out);
    NodeParams direct;
    direct.frequency = kFreq;
    direct.waveform = Waveform::Sawtooth;
    CHECK(maxAbsDiff(out, renderOsc(direct)) < 1e-6);
  }

  SUBCASE("parse: an authored periodicWave shapes the parsed oscillator") {
    // (c) parse path: the 4.1 field reads and drives the oscillator.
    const char *xml =
        "<?xml version='1.0' encoding='UTF-8'?>"
        "<X3D profile='Immersive' version='4.1'>"
        "<Scene>"
        "<AudioDestination>"
        "<OscillatorSource DEF='TONE' frequency='220'>"
        "<PeriodicWave containerField='periodicWave' type='CUSTOM' "
        "optionsReal='0 0' optionsImag='0 1'/>"
        "</OscillatorSource>"
        "</AudioDestination>"
        "</Scene></X3D>";
    X3DDocument doc = x3d::codec::parseDocument(xml);
    CHECK(doc.readerWarnings.empty());
    auto *osc = dynamic_cast<OscillatorSource *>(doc.scene.resolve("TONE").get());
    REQUIRE(osc != nullptr);
    auto *wave = dynamic_cast<PeriodicWave *>(osc->getPeriodicWave().get());
    REQUIRE(wave != nullptr);
    CHECK(wave->getOptionsImag().size() == 2);
    CHECK_FALSE(osc->getIS());

    auto rec = std::make_shared<RecordingBackend>();
    SoundSystem sys(rec);
    X3DExecutionContext ctx;
    sys.attach(doc.scene.rootNodes[0].get(), ctx);
    const NodeParams *params = nullptr;
    for (const auto &cr : rec->creates)
      if (cr.kind == NodeKind::Oscillator) params = &cr.params;
    REQUIRE(params != nullptr);
    CHECK(params->waveform == Waveform::Custom);
    CHECK(params->optionsImag.size() == 2);
  }
}
