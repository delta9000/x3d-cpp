// MiniaudioBackend.hpp
// AudioBackend implementation over miniaudio's headless ma_node_graph.
// Engine-free: NO ma_device_init, NO ma_engine_init — headless render only.
//
// pImpl: this header is miniaudio-FREE (no miniaudio.h include) so consumers
// that link x3d_miniaudio never see miniaudio types. All ma_* types, #defines,
// and MINIAUDIO_IMPLEMENTATION live exclusively in MiniaudioBackend.cpp.
// Mirrors BuiltinDspBackend.hpp in structure and contract.
//
// Synthesis chain (v1): Oscillator → Biquad → Gain → Destination.
//   Oscillator  : ma_waveform wrapped in ma_data_source_node (sine/square/saw/tri).
//   Biquad      : ma_lpf_node / ma_hpf_node / ma_bpf_node (lowpass/highpass/bandpass).
//   Gain        : ma_splitter_node (1→1) with ma_node_set_output_bus_volume.
//   Destination : ma_node_graph endpoint (ma_node_graph_get_endpoint).
//
// connect(dst, src) → ma_node_attach_output_bus(src, 0, dst, 0).
// render()         → ma_node_graph_read_pcm_frames(...) — no audio device.
//
// Lazy init: miniaudio nodes require sampleRate at construction but the seam
// only supplies it at render() time. All nodes are kept PENDING after
// createNode(); the first render() call flushes PENDING nodes to their
// respective ma_* nodes at the given sampleRate, then wires the deferred
// connect() calls.
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
