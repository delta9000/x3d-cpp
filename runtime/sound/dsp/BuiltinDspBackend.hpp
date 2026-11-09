// Dependency-free AudioBackend implementation. DSP state is private to Impl.
// See ADR-0026.
#ifndef X3D_RUNTIME_BUILTIN_DSP_BACKEND_HPP
#define X3D_RUNTIME_BUILTIN_DSP_BACKEND_HPP

#include "AudioBackend.hpp"

#include <memory>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Dependency-free DSP backend implementing AudioBackend.
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
