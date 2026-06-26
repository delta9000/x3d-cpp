// BuiltinDspBackend.hpp
// A dependency-free DSP backend for the AudioBackend seam — the default audio
// engine. No external library: pure math (oscillators, RBJ biquad, gain, summing
// destination), fitting the project's no-external-libs discipline (the Jolt /
// Duktape analog is the flag-gated production backend, here LabSound — deferred).
//
// pImpl: this header is DSP-FREE (no coefficients, no phase, no filter state) so
// consumers that link the backend never see the DSP internals; all of that lives
// in BuiltinDspBackend.cpp. The header exposes only the AudioBackend interface +
// an opaque owned Impl, mirroring JoltBackend.hpp.
#ifndef X3D_RUNTIME_BUILTIN_DSP_BACKEND_HPP
#define X3D_RUNTIME_BUILTIN_DSP_BACKEND_HPP

#include "AudioBackend.hpp"

#include <memory>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Dependency-free DSP backend implementing AudioBackend (mono, v1).
 * @details Owns an audio-processing graph of oscillators / biquads / gains /
 *          a summing destination; render() does a topological pull from the
 *          destination through the graph for `frames` samples at sampleRate.
 *          Deterministic: a fixed graph + params + sampleRate yields a
 *          byte-reproducible buffer. All DSP is in BuiltinDspBackend.cpp.
 */
class BuiltinDspBackend : public AudioBackend {
public:
  BuiltinDspBackend();
  ~BuiltinDspBackend() override;

  NodeHandle createNode(NodeKind kind, const NodeParams &params) override;
  void connect(NodeHandle dst, NodeHandle src) override;
  void setParam(NodeHandle node, Param param, float value) override;
  void render(NodeHandle destination, int frames, float sampleRate,
              std::vector<float> &out) override;
  void renderStereo(NodeHandle destination, int frames, float sampleRate,
                    std::vector<float> &outLR) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_BUILTIN_DSP_BACKEND_HPP
