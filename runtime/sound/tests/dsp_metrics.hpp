// dsp_metrics.hpp — shared DSP measurement helpers for audio swap-tests.
// Used by sound_system_test.cpp and sound_swap_test.cpp.
// No DSP implementation lives here — only measurement utilities (RMS + Goertzel).
#ifndef X3D_RUNTIME_SOUND_TESTS_DSP_METRICS_HPP
#define X3D_RUNTIME_SOUND_TESTS_DSP_METRICS_HPP

#include <cmath>
#include <vector>

namespace x3d::test {

/**
 * @brief Root-mean-square of a mono sample buffer.
 * @details Returns 0.0 for an empty buffer.
 */
static inline double rms(const std::vector<float> &b) {
  if (b.empty()) return 0.0;
  double s = 0.0;
  for (float v : b) s += static_cast<double>(v) * v;
  return std::sqrt(s / static_cast<double>(b.size()));
}

/**
 * @brief Goertzel magnitude of `targetHz` in buffer `b` sampled at `sampleRate`.
 * @details A single-bin DFT — proves a tone is present without a full FFT
 *          dependency. Returns 0.0 for an empty buffer.
 */
static inline double goertzel(const std::vector<float> &b, double targetHz,
                               double sampleRate) {
  if (b.empty()) return 0.0;
  int N = static_cast<int>(b.size());
  double k = std::round(targetHz / sampleRate * N);
  double w = 2.0 * 3.14159265358979323846 * k / N;
  double cosw = std::cos(w);
  double coeff = 2.0 * cosw;
  double s0 = 0, s1 = 0, s2 = 0;
  for (int i = 0; i < N; ++i) {
    s0 = coeff * s1 - s2 + b[static_cast<std::size_t>(i)];
    s2 = s1;
    s1 = s0;
  }
  double real = s1 - s2 * cosw;
  double imag = s2 * std::sin(w);
  return std::sqrt(real * real + imag * imag) / (N / 2.0);
}

/**
 * @brief RMS of the L-channel (even indices) of an interleaved stereo buffer.
 */
static inline double rmsL(const std::vector<float> &lr) {
  if (lr.empty()) return 0.0;
  double s = 0.0;
  std::size_t n = 0;
  for (std::size_t i = 0; i < lr.size(); i += 2, ++n)
    s += static_cast<double>(lr[i]) * lr[i];
  return n > 0 ? std::sqrt(s / static_cast<double>(n)) : 0.0;
}

/**
 * @brief RMS of the R-channel (odd indices) of an interleaved stereo buffer.
 */
static inline double rmsR(const std::vector<float> &lr) {
  if (lr.empty()) return 0.0;
  double s = 0.0;
  std::size_t n = 0;
  for (std::size_t i = 1; i < lr.size(); i += 2, ++n)
    s += static_cast<double>(lr[i]) * lr[i];
  return n > 0 ? std::sqrt(s / static_cast<double>(n)) : 0.0;
}

/**
 * @brief Total RMS across both channels of an interleaved stereo buffer.
 * @details All-samples RMS: applies the same sqrt(sum(v²)/N) formula as mono
 *          `rms()`, treating every interleaved sample (L and R alike) equally.
 *          This is intentional — it measures total signal energy per sample,
 *          not a per-channel average.
 */
static inline double rmsStereo(const std::vector<float> &lr) {
  return rms(lr);  // rms treats all samples equally; same formula
}

} // namespace x3d::test

#endif // X3D_RUNTIME_SOUND_TESTS_DSP_METRICS_HPP
