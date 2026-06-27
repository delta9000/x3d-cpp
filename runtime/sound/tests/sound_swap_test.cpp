// sound_swap_test.cpp — Headless SWAP-TEST: the Audio seam genericity proof.
//
// Renders the SAME SoundSystem-built §16 audio graphs through BOTH backends
// (BuiltinDspBackend + MiniaudioBackend) with no audio device and asserts:
//
//   SYNTHESIS fixtures (numeric tolerance — calibrated from Task-1 spike):
//     F1: Mono tone  — Oscillator(440, sine, gain=1) → Destination;
//         Goertzel@440 of both backends agree within ±5%.
//         RMS agree within ±2%. Off-frequency bin (1kHz) ≪ tone on both.
//     F2: Mono lowpass — Oscillator(440) → Biquad(Lowpass, 1kHz) → Destination;
//         Both backends attenuate stopband(8kHz source) to <2% of passband.
//         Goertzel@passband agree within ±15%.
//
//   SPATIAL fixtures (STRUCTURAL — BuiltinDsp equal-power vs ma_spatializer;
//     rendered via SoundSystem.renderStereo()):
//     F3: Oscillator → Panner → stereo Destination, listener at origin -Z.
//         For EACH backend independently (NOT cross-backend numeric equality):
//           - hard-left  ⇒ rmsL > rmsR (correct ear dominant)
//           - hard-right ⇒ rmsR > rmsL
//           - centered   ⇒ |rmsL − rmsR| < 1% of max (non-vacuous: silence fails)
//           - monotonic distance falloff: total stereo energy strictly decreases
//             (ref→mid→far) for each distance model; source on centered on-axis
//             (0,0,-dist) so panning stays symmetric and only distance varies.
//           - total stereo energy at refDist (centered): symmetric ±40% of monoRms/√2
//             (equal-power ref = monoRms/√2; miniaudio amplitude law lands at ~0.71×ref;
//              both lower AND upper bounds real; near-zero fails lower bound)
//         Distance models LINEAR, INVERSE, EXPONENTIAL all exercised.
//         Cross-backend STRUCTURAL assertion: BOTH backends put the louder
//           channel on the SAME side (sign agreement). Numbers are NOT compared
//           (different panning laws: equal-power vs ma_spatializer amplitude law).
//
// Anti-tautology guardrail (verified in comments): each assertion fails if a
// backend's DSP were stubbed to echo input (L==R would break ear-sign;
// no synthesis would break Goertzel).
//
// Style: hand-rolled main() + CHECK macro + g_failures counter (same as
// sound_system_test.cpp). Shared metric helpers from dsp_metrics.hpp.

#include "AudioBackend.hpp"
#include "SoundSystem.hpp"
#include "dsp/BuiltinDspBackend.hpp"
#include "miniaudio/MiniaudioBackend.hpp"
#include "tests/dsp_metrics.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/SpatialSound.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using x3d::test::rms;
using x3d::test::goertzel;
using x3d::test::rmsL;
using x3d::test::rmsR;
using x3d::test::rmsStereo;

// ── test harness ────────────────────────────────────────────────────────────

static int g_failures = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);    \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

// ── constants ────────────────────────────────────────────────────────────────

// Thresholds calibrated from Task-1 spike (audio-spike-report.md) + observed
// values from F2 filtered passband runs:
//   Raw tone RMS agreement:     ≤0.01%  → kRmsTol  = ±2%  (200× headroom)
//   Raw tone Goertzel@440:      ≤0.00%  → kGoertzelTol = ±5%  (for F1, raw only)
//   Filtered passband Goertzel: ~9–10%  → kGoertzelFilteredTol = ±15%
//     (RBJ biquad vs miniaudio lpf_node have different in-band rolloff with
//     default Q=1 at 440/1000Hz; both produce strong signal, but magnitudes
//     differ between implementations — that's expected, not a regression)
//   Stopband ratio (observed ~1.28-1.38%) → kStopbandRatio = 2%
//     (both backends strongly attenuate; 1% was too tight for the 8kHz case)
static constexpr double kRmsTol              = 0.02;  // ±2%  RMS (raw tone)
static constexpr double kGoertzelTol         = 0.05;  // ±5%  Goertzel (raw tone, F1)
static constexpr double kGoertzelFilteredTol = 0.15;  // ±15% Goertzel (filtered passband, F2)
static constexpr double kStopbandRatio       = 0.02;  // <2%  stopband/passband ratio
static constexpr double kSpatialEnergy       = 0.40;  // ±40% stereo vs mono/√2 energy
// kSpatialEnergy rationale: at a centered position the all-samples rmsStereo
// equals monoRms/√2 for equal-power panning.  miniaudio uses an amplitude law
// that produces ~monoRms/2 = (monoRms/√2)/√2 ≈ 0.707×(monoRms/√2), so ±40%
// of the equal-power reference monoRms/√2 covers both backends with ~10% headroom.

static constexpr float  kSR    = 48000.0f;
static constexpr int    kFrames = 4096;

// ── graph helpers ────────────────────────────────────────────────────────────

struct MonoChain {
  std::shared_ptr<OscillatorSource> osc;
  std::shared_ptr<BiquadFilter>     biquad;   // nullptr when absent
  std::shared_ptr<AudioDestination> dest;
};

// Osc(freq, gain=1) → Destination  [no biquad]
static MonoChain buildToneChain(float oscFreq) {
  MonoChain c;
  c.osc = std::make_shared<OscillatorSource>();
  c.osc->setFrequency(oscFreq);
  c.dest = std::make_shared<AudioDestination>();
  c.dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.osc)});
  return c;
}

// Osc(freq) → Biquad(Lowpass, cutoff) → Destination
static MonoChain buildLowpassChain(float oscFreq, float cutoff) {
  MonoChain c;
  c.osc = std::make_shared<OscillatorSource>();
  c.osc->setFrequency(oscFreq);
  c.biquad = std::make_shared<BiquadFilter>();
  c.biquad->setType(BiquadTypeFilterChoices::LOWPASS);
  c.biquad->setFrequency(cutoff);
  c.biquad->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.osc)});
  c.dest = std::make_shared<AudioDestination>();
  c.dest->setChildren(MFNode{std::static_pointer_cast<X3DNode>(c.biquad)});
  return c;
}

// Attach + render mono through a backend, returns the buffer.
static std::vector<float> renderMono(std::shared_ptr<AudioBackend> backend,
                                     AudioDestination *dest) {
  SoundSystem sys(backend);
  X3DExecutionContext ctx;
  sys.attach(dest, ctx);
  std::vector<float> buf;
  sys.render(kFrames, kSR, buf);
  return buf;
}

// ── spatial helpers ──────────────────────────────────────────────────────────

// Build a panner NodeParams for a given source position + listener at origin -Z.
// distance model, ref, max, rolloff are caller-provided.
static NodeParams makePannerParams(float sx, float sy, float sz,
                                   DistanceModel dm,
                                   float refDist, float maxDist,
                                   float rolloff) {
  NodeParams p;
  p.sourcePosition[0]   = sx; p.sourcePosition[1]   = sy; p.sourcePosition[2]   = sz;
  p.listenerPosition[0] = 0.0f; p.listenerPosition[1] = 0.0f; p.listenerPosition[2] = 0.0f;
  p.listenerForward[0]  = 0.0f; p.listenerForward[1]  = 0.0f; p.listenerForward[2]  = -1.0f;
  p.listenerUp[0]       = 0.0f; p.listenerUp[1]       = 1.0f; p.listenerUp[2]       = 0.0f;
  p.distanceModel       = dm;
  p.referenceDistance   = refDist;
  p.maxDistance         = maxDist;
  p.rolloffFactor       = rolloff;
  return p;
}

// Build a direct (NodeParams-level) panner graph and render stereo.
// Bypasses SoundSystem to directly control the panner params.
static std::vector<float> renderDirectPannerStereo(
    std::shared_ptr<AudioBackend> backend,
    const NodeParams &pannerP, float oscFreq) {
  NodeParams oscP;
  oscP.frequency = oscFreq;
  oscP.gain = 1.0f;
  NodeHandle hOsc = backend->createNode(NodeKind::Oscillator, oscP);

  NodeHandle hPanner = backend->createNode(NodeKind::Panner, pannerP);

  NodeParams destP;
  destP.maxChannelCount = 2;
  NodeHandle hDest = backend->createNode(NodeKind::Destination, destP);

  backend->connect(hPanner, hOsc);
  backend->connect(hDest, hPanner);

  std::vector<float> lr;
  backend->renderStereo(hDest, kFrames, kSR, lr);
  return lr;
}

// ── test fixtures ────────────────────────────────────────────────────────────

// F1: Mono 440 Hz tone — both backends agree on RMS and Goertzel@440.
// Anti-tautology: a stub echoing input produces RMS≈0 (no tone synthesis)
// and Goertzel@440≈0 — both assertions fail.
static void testF1_MonoTone() {
  MonoChain chain = buildToneChain(440.0f);

  auto builtin = std::make_shared<BuiltinDspBackend>();
  auto ma      = std::make_shared<x3d::runtime::miniaudio::MiniaudioBackend>();

  std::vector<float> bufA = renderMono(builtin, chain.dest.get());

  // Build fresh chain for ma (SoundSystem consumes the dest graph)
  MonoChain chain2 = buildToneChain(440.0f);
  std::vector<float> bufB = renderMono(ma, chain2.dest.get());

  double rmsA = rms(bufA), rmsB = rms(bufB);
  double g440A = goertzel(bufA, 440.0, kSR);
  double g440B = goertzel(bufB, 440.0, kSR);
  double g1kA  = goertzel(bufA, 1000.0, kSR);
  double g1kB  = goertzel(bufB, 1000.0, kSR);

  std::fprintf(stderr,
               "[F1 mono tone] builtin: rms=%.4f goertzel@440=%.4f\n"
               "               ma:      rms=%.4f goertzel@440=%.4f\n",
               rmsA, g440A, rmsB, g440B);

  // Both backends synthesise a real tone.
  CHECK(g440A > 0.3, "F1/builtin: Goertzel@440 is strong (>0.3)");
  CHECK(g440B > 0.3, "F1/ma:      Goertzel@440 is strong (>0.3)");

  // RMS agrees within ±2%.
  double maxRms = std::max(rmsA, rmsB);
  CHECK(maxRms > 1e-9, "F1: at least one backend has non-zero RMS");
  double rmsRelDiff = std::fabs(rmsA - rmsB) / maxRms;
  std::fprintf(stderr, "[F1] rms_rel_diff=%.4f%%\n", rmsRelDiff * 100.0);
  CHECK(rmsRelDiff <= kRmsTol, "F1: RMS agrees within ±2%");

  // Goertzel@440 agrees within ±5%.
  double maxG440 = std::max(g440A, g440B);
  double g440RelDiff = std::fabs(g440A - g440B) / maxG440;
  std::fprintf(stderr, "[F1] goertzel440_rel_diff=%.4f%%\n", g440RelDiff * 100.0);
  CHECK(g440RelDiff <= kGoertzelTol, "F1: Goertzel@440 agrees within ±5%");

  // Off-frequency bin (1kHz) ≪ tone on both backends (< 2% of 440Hz tone).
  CHECK(g1kA < g440A * kStopbandRatio,
        "F1/builtin: 1kHz bin ≪ 440Hz tone (< 2%)");
  CHECK(g1kB < g440B * kStopbandRatio,
        "F1/ma:      1kHz bin ≪ 440Hz tone (< 2%)");
}

// F2: Lowpass filter — passband passes, stopband strongly attenuated.
// Variant (a): 440 Hz tone through 1kHz lowpass → passband.
// Variant (b): 8kHz tone through 1kHz lowpass → stopband < 2% of passband.
// Anti-tautology: a stub echoing input has stopband≈passband, so the
// stopband < 2% check fails; the strong-tone checks also fail.
static void testF2_MonoLowpass() {
  // ── variant (a): passband — 440 Hz through 1kHz lowpass ──
  MonoChain passA_chain = buildLowpassChain(440.0f, 1000.0f);
  MonoChain passB_chain = buildLowpassChain(440.0f, 1000.0f);

  auto builtinA = std::make_shared<BuiltinDspBackend>();
  auto maA      = std::make_shared<x3d::runtime::miniaudio::MiniaudioBackend>();

  std::vector<float> bufPassA = renderMono(builtinA, passA_chain.dest.get());
  std::vector<float> bufPassB = renderMono(maA, passB_chain.dest.get());

  double g440A = goertzel(bufPassA, 440.0, kSR);
  double g440B = goertzel(bufPassB, 440.0, kSR);

  std::fprintf(stderr,
               "[F2 lowpass passband] builtin goertzel@440=%.4f  ma goertzel@440=%.4f\n",
               g440A, g440B);

  // Both backends pass the 440 Hz tone through 1kHz lowpass.
  CHECK(g440A > 0.3, "F2/builtin: passband 440 Hz passes (Goertzel > 0.3)");
  CHECK(g440B > 0.3, "F2/ma:      passband 440 Hz passes (Goertzel > 0.3)");

  // Goertzel@passband agrees within ±15%.
  // The two biquad implementations (RBJ biquad vs miniaudio lpf_node) have
  // slightly different in-band frequency response at 440Hz/1kHz cutoff with
  // default Q=1, so a wider tolerance is needed than for a raw unfiltered tone.
  double maxG = std::max(g440A, g440B);
  double gRelDiff = std::fabs(g440A - g440B) / maxG;
  std::fprintf(stderr, "[F2] goertzel_passband_rel_diff=%.4f%%\n", gRelDiff * 100.0);
  CHECK(gRelDiff <= kGoertzelFilteredTol,
        "F2: passband Goertzel@440 agrees within ±15% (filter-implementation delta)");

  // ── variant (b): stopband — 8kHz tone through 1kHz lowpass ──
  MonoChain stopA_chain = buildLowpassChain(8000.0f, 1000.0f);
  MonoChain stopB_chain = buildLowpassChain(8000.0f, 1000.0f);

  auto builtinB = std::make_shared<BuiltinDspBackend>();
  auto maB      = std::make_shared<x3d::runtime::miniaudio::MiniaudioBackend>();

  std::vector<float> bufStopA = renderMono(builtinB, stopA_chain.dest.get());
  std::vector<float> bufStopB = renderMono(maB, stopB_chain.dest.get());

  double rmsPassA = rms(bufPassA), rmsStopA = rms(bufStopA);
  double rmsPassB = rms(bufPassB), rmsStopB = rms(bufStopB);

  std::fprintf(stderr,
               "[F2 lowpass stopband] builtin: passband_rms=%.4f stopband_rms=%.5f ratio=%.4f\n"
               "                     ma:      passband_rms=%.4f stopband_rms=%.5f ratio=%.4f\n",
               rmsPassA, rmsStopA, rmsPassA > 0 ? rmsStopA / rmsPassA : 0.0,
               rmsPassB, rmsStopB, rmsPassB > 0 ? rmsStopB / rmsPassB : 0.0);

  // Both backends attenuate 8kHz stopband to <2% of 440Hz passband.
  // Observed values: builtin ~1.28%, ma ~1.38% — both well below 2%.
  CHECK(rmsStopA < rmsPassA * kStopbandRatio,
        "F2/builtin: 8kHz stopband < 2% of 440Hz passband");
  CHECK(rmsStopB < rmsPassB * kStopbandRatio,
        "F2/ma:      8kHz stopband < 2% of 440Hz passband");
}

// F3: Spatial structural test for one distance model.
// For EACH backend: assert ear-sign (hard-left, hard-right, centre), monotonic
// distance falloff, and stereo energy ≈ mono.
// Cross-backend: assert BOTH backends put louder channel on SAME side (sign).
// Anti-tautology: a stub echoing input produces L==R (breaking ear-sign
// assertions) and no synthesis (breaking energy assertions).
static void testF3_Spatial(DistanceModel dm, const char *dmName) {
  std::fprintf(stderr, "\n[F3 spatial / %s]\n", dmName);

  // Use referenceDistance=1, maxDistance=10, rolloff=1 for all models.
  // Source positions:
  //   hard-left  = (-5, 0, 0)  — far left, same Z as listener
  //   hard-right = ( 5, 0, 0)  — far right
  //   center     = ( 0, 0, -5) — in front
  //   at refDist = ( 1, 0, 0)  — reference distance (used for energy baseline)
  //   at dist5   = ( 5, 0, 0)  — 5× reference distance

  const float kRefDist = 1.0f;
  const float kMaxDist = 10.0f;

  // Helper: render stereo for both backends at given source position.
  auto renderBothStereo = [&](float sx, float sy, float sz,
                               std::vector<float> &lrA,
                               std::vector<float> &lrB) {
    NodeParams pp = makePannerParams(sx, sy, sz, dm, kRefDist, kMaxDist, 1.0f);
    auto backendA = std::make_shared<BuiltinDspBackend>();
    auto backendB = std::make_shared<x3d::runtime::miniaudio::MiniaudioBackend>();
    lrA = renderDirectPannerStereo(backendA, pp, 440.0f);
    lrB = renderDirectPannerStereo(backendB, pp, 440.0f);
  };

  std::vector<float> lrA, lrB;

  // ── hard-left: source at (-5, 0, 0) ──
  renderBothStereo(-5.0f, 0.0f, 0.0f, lrA, lrB);
  double lLA = rmsL(lrA), lRA = rmsR(lrA);
  double lLB = rmsL(lrB), lRB = rmsR(lrB);
  std::fprintf(stderr, "  hard-left builtin: L=%.4f R=%.4f  ma: L=%.4f R=%.4f\n",
               lLA, lRA, lLB, lRB);
  CHECK(lLA > lRA, "F3/builtin/hard-left: L channel dominates");
  CHECK(lLB > lRB, "F3/ma/hard-left:      L channel dominates");
  // Cross-backend sign agreement.
  bool leftSignA = (lLA > lRA);
  bool leftSignB = (lLB > lRB);
  CHECK(leftSignA == leftSignB,
        "F3/cross: hard-left — both backends agree on dominant channel (L)");

  // ── hard-right: source at (+5, 0, 0) ──
  renderBothStereo(5.0f, 0.0f, 0.0f, lrA, lrB);
  double rLA = rmsL(lrA), rRA = rmsR(lrA);
  double rLB = rmsL(lrB), rRB = rmsR(lrB);
  std::fprintf(stderr, "  hard-right builtin: L=%.4f R=%.4f  ma: L=%.4f R=%.4f\n",
               rLA, rRA, rLB, rRB);
  CHECK(rRA > rLA, "F3/builtin/hard-right: R channel dominates");
  CHECK(rRB > rLB, "F3/ma/hard-right:      R channel dominates");
  bool rightSignA = (rRA > rLA);
  bool rightSignB = (rRB > rLB);
  CHECK(rightSignA == rightSignB,
        "F3/cross: hard-right — both backends agree on dominant channel (R)");

  // ── centered: source at (0, 0, -refDist) ──
  renderBothStereo(0.0f, 0.0f, -kRefDist, lrA, lrB);
  double cLA = rmsL(lrA), cRA = rmsR(lrA);
  double cLB = rmsL(lrB), cRB = rmsR(lrB);
  double cMaxA = std::max(cLA, cRA);
  double cMaxB = std::max(cLB, cRB);
  std::fprintf(stderr, "  centered  builtin: L=%.4f R=%.4f  |L-R|/max=%.4f\n"
               "            ma:      L=%.4f R=%.4f  |L-R|/max=%.4f\n",
               cLA, cRA,
               cMaxA > 1e-9 ? std::fabs(cLA - cRA) / cMaxA : 0.0,
               cLB, cRB,
               cMaxB > 1e-9 ? std::fabs(cLB - cRB) / cMaxB : 0.0);
  // Non-zero check first: silence must fail here, not pass vacuously below.
  CHECK(cMaxA > 0.01, "F3/builtin/centered: non-zero output");
  CHECK(cMaxB > 0.01, "F3/ma/centered:      non-zero output");
  // Symmetry check only runs on real (non-silent) output, so silence fails
  // the non-zero check above and never reaches here vacuously.
  if (cMaxA > 1e-9)
    CHECK(std::fabs(cLA - cRA) / cMaxA < 0.01,
          "F3/builtin/centered: |L-R| < 1% of max (symmetric)");
  if (cMaxB > 1e-9)
    CHECK(std::fabs(cLB - cRB) / cMaxB < 0.01,
          "F3/ma/centered:      |L-R| < 1% of max (symmetric)");

  // ── monotonic distance falloff: three distances (1, 3, 6 × refDist) ──
  // Source on the centered on-axis position (0, 0, -dist) so panning stays
  // symmetric (L≈R) and ONLY distance attenuation varies.  Using the +X axis
  // (hard-right) was incorrect: it suppressed the L channel and forced the
  // energy bound to be one-sided.
  // For LINEAR model: at dist=10 (maxDist with rolloff=1) gain→0.
  // For INVERSE/EXPONENTIAL: gain decreases monotonically with distance.
  auto totalStereoRms = [&](float dist) {
    NodeParams pp = makePannerParams(0.0f, 0.0f, -dist, dm, kRefDist, kMaxDist, 1.0f);
    auto bA = std::make_shared<BuiltinDspBackend>();
    auto bB = std::make_shared<x3d::runtime::miniaudio::MiniaudioBackend>();
    auto lrA2 = renderDirectPannerStereo(bA, pp, 440.0f);
    auto lrB2 = renderDirectPannerStereo(bB, pp, 440.0f);
    return std::make_pair(rmsStereo(lrA2), rmsStereo(lrB2));
  };

  auto [rmsRef_A, rmsRef_B] = totalStereoRms(kRefDist);      // dist = 1
  auto [rmsMid_A, rmsMid_B] = totalStereoRms(3.0f);          // dist = 3
  auto [rmsFar_A, rmsFar_B] = totalStereoRms(6.0f);          // dist = 6

  std::fprintf(stderr,
               "  falloff builtin: d=1→%.4f  d=3→%.4f  d=6→%.4f\n"
               "          ma:      d=1→%.4f  d=3→%.4f  d=6→%.4f\n",
               rmsRef_A, rmsMid_A, rmsFar_A,
               rmsRef_B, rmsMid_B, rmsFar_B);

  // Monotonic: ref > mid > far.
  CHECK(rmsRef_A > rmsMid_A,
        "F3/builtin: distance falloff monotonic (d=1 > d=3)");
  CHECK(rmsMid_A > rmsFar_A,
        "F3/builtin: distance falloff monotonic (d=3 > d=6)");
  CHECK(rmsRef_B > rmsMid_B,
        "F3/ma: distance falloff monotonic (d=1 > d=3)");
  CHECK(rmsMid_B > rmsFar_B,
        "F3/ma: distance falloff monotonic (d=3 > d=6)");

  // Gain < 1 beyond referenceDistance (i.e., far < ref).
  CHECK(rmsFar_A < rmsRef_A, "F3/builtin: gain < 1 at d=6 > referenceDistance");
  CHECK(rmsFar_B < rmsRef_B, "F3/ma:      gain < 1 at d=6 > referenceDistance");

  // ── energy preservation: symmetric ±40% of monoRms/√2 (both backends, at refDist) ──
  // Source at (0, 0, -refDist) — centered, directly in front of the listener.
  // For an equal-power centered pan: rmsL = rmsR = monoRms/√2, so:
  //   rmsStereo = sqrt((rmsL² + rmsR²) / 2) = monoRms/√2  (equal-power reference)
  // miniaudio's ma_spatializer uses an amplitude law, producing
  //   rmsL = rmsR ≈ monoRms/2 → rmsStereo ≈ monoRms/2 = (monoRms/√2)/√2
  // so ±40% of monoRms/√2 is the tightest honest symmetric bound covering both
  // implementations with ~10% headroom (observed: builtin=1.00×ref, ma≈0.71×ref).
  // The real muted-channel guard is the symmetry check above (|L-R|/max < 1%);
  // this bound additionally catches near-zero and runaway-gain pathologies.
  // rmsRef_A/B were measured at (0,0,-1) in the falloff sweep above — reuse them.
  {
    MonoChain mono = buildToneChain(440.0f);
    auto bMono = std::make_shared<BuiltinDspBackend>();
    std::vector<float> monoOut = renderMono(bMono, mono.dest.get());
    double monoRms = rms(monoOut);
    // Equal-power reference: rmsStereo = monoRms/√2 for centred equal-power pan.
    const double kSqrt2 = 1.41421356237309504880;
    double refEnergy = monoRms / kSqrt2;

    std::fprintf(stderr,
                 "  energy(centered): mono_rms=%.4f  ref_energy(mono/√2)=%.4f\n"
                 "                   stereo_at_ref: builtin=%.4f (%.2f×ref)  ma=%.4f (%.2f×ref)\n"
                 "                   expected_band: [%.4f, %.4f]\n",
                 monoRms, refEnergy,
                 rmsRef_A, rmsRef_A / refEnergy,
                 rmsRef_B, rmsRef_B / refEnergy,
                 refEnergy * (1.0 - kSpatialEnergy),
                 refEnergy * (1.0 + kSpatialEnergy));

    // Symmetric ±40% of monoRms/√2 — both lower AND upper bounds are real.
    double lo = refEnergy * (1.0 - kSpatialEnergy);
    double hi = refEnergy * (1.0 + kSpatialEnergy);
    CHECK(rmsRef_A >= lo,
          "F3/builtin: centered stereo energy >= mono/√2 * 0.60 (lower bound)");
    CHECK(rmsRef_A <= hi,
          "F3/builtin: centered stereo energy <= mono/√2 * 1.40 (upper bound)");
    CHECK(rmsRef_B >= lo,
          "F3/ma:      centered stereo energy >= mono/√2 * 0.60 (lower bound)");
    CHECK(rmsRef_B <= hi,
          "F3/ma:      centered stereo energy <= mono/√2 * 1.40 (upper bound)");
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::fprintf(stderr,
               "=== sound_swap_test: BuiltinDsp vs miniaudio headless swap-test ===\n"
               "    %d frames @ %.0f Hz\n", kFrames, static_cast<double>(kSR));

  // ── F1: Synthesis — mono tone numeric tolerance ──
  std::fprintf(stderr, "\n--- F1: Mono 440 Hz tone ---\n");
  testF1_MonoTone();

  // ── F2: Synthesis — lowpass filter numeric tolerance ──
  std::fprintf(stderr, "\n--- F2: Mono lowpass filter ---\n");
  testF2_MonoLowpass();

  // ── F3: Spatial structural (three distance models) ──
  std::fprintf(stderr, "\n--- F3: Spatial (LINEAR) ---\n");
  testF3_Spatial(DistanceModel::Linear, "LINEAR");

  std::fprintf(stderr, "\n--- F3: Spatial (INVERSE) ---\n");
  testF3_Spatial(DistanceModel::Inverse, "INVERSE");

  std::fprintf(stderr, "\n--- F3: Spatial (EXPONENTIAL) ---\n");
  testF3_Spatial(DistanceModel::Exponential, "EXPONENTIAL");

  std::fprintf(stderr, "\n");
  if (g_failures == 0)
    std::fprintf(stderr, "sound_swap_test: ALL PASS\n");
  else
    std::fprintf(stderr, "sound_swap_test: %d FAILURE(S)\n", g_failures);

  return g_failures == 0 ? 0 : 1;
}
