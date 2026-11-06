// Headless miniaudio AudioBackend. Library types stay private to Impl.
// Nodes need the render sample rate at construction, so createNode and connect
// are deferred until the first render. See ADR-0026.
#ifndef X3D_RUNTIME_MINIAUDIO_BACKEND_HPP
#define X3D_RUNTIME_MINIAUDIO_BACKEND_HPP

#include "AudioBackend.hpp"

#include <memory>
#include <vector>

namespace x3d::runtime::miniaudio {

/**
 * @brief AudioBackend implementation over a headless miniaudio ma_node_graph.
 * @details No audio device is opened. render() pulls samples through the
 *          graph via ma_node_graph_read_pcm_frames into a std::vector<float>.
 *          All miniaudio types are confined to MiniaudioBackend.cpp (pImpl).
 */
class MiniaudioBackend : public x3d::runtime::AudioBackend {
public:
  MiniaudioBackend();
  ~MiniaudioBackend() override;

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

} // namespace x3d::runtime::miniaudio

#endif // X3D_RUNTIME_MINIAUDIO_BACKEND_HPP
