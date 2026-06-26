// RecordingBackend.hpp
// A test AudioBackend that records every seam op (createNode / connect /
// setParam) into an inspectable list, for deterministic graph-construction
// assertions. It does NO DSP — render() emits silence. It is the audio analog of
// inspecting the recorded backend calls: it proves SoundSystem reads the §16
// `children` graph into the correct nodes + connections + initial params, in a
// deterministic op order, independent of any DSP correctness.
#ifndef X3D_RUNTIME_RECORDING_BACKEND_HPP
#define X3D_RUNTIME_RECORDING_BACKEND_HPP

#include "AudioBackend.hpp"

#include <vector>

namespace x3d::runtime {

/**
 * @brief An AudioBackend that records the ops it receives (no DSP).
 * @details createNode() mints sequential handles (1,2,3,...) and records the
 *          kind + params; connect()/setParam() record their arguments. Tests
 *          inspect `ops` (and the typed convenience vectors) to assert the graph
 *          SoundSystem built. render() returns silence.
 */
class RecordingBackend : public AudioBackend {
public:
  /** @brief One recorded createNode call. */
  struct CreateOp {
    NodeHandle handle;
    NodeKind kind;
    NodeParams params;
  };
  /** @brief One recorded connect call (src feeds INTO dst). */
  struct ConnectOp {
    NodeHandle dst;
    NodeHandle src;
  };
  /** @brief One recorded setParam call. */
  struct SetParamOp {
    NodeHandle node;
    Param param;
    float value;
  };

  /** @brief A tagged record of any single op, in receive order (determinism). */
  struct Op {
    enum class Kind { Create, Connect, SetParam } kind;
    NodeHandle a = kInvalidNodeHandle;  // create:handle  connect:dst  set:node
    NodeHandle b = kInvalidNodeHandle;  // connect:src
    NodeKind nodeKind = NodeKind::Oscillator;
    Param param = Param::Gain;
    float value = 0.0f;
  };

  NodeHandle createNode(NodeKind kind, const NodeParams &params) override {
    NodeHandle h = ++lastHandle_;
    creates.push_back(CreateOp{h, kind, params});
    ops.push_back(Op{Op::Kind::Create, h, kInvalidNodeHandle, kind,
                     Param::Gain, 0.0f});
    return h;
  }

  void connect(NodeHandle dst, NodeHandle src) override {
    connects.push_back(ConnectOp{dst, src});
    ops.push_back(
        Op{Op::Kind::Connect, dst, src, NodeKind::Oscillator, Param::Gain, 0.0f});
  }

  void setParam(NodeHandle node, Param param, float value) override {
    setParams.push_back(SetParamOp{node, param, value});
    ops.push_back(Op{Op::Kind::SetParam, node, kInvalidNodeHandle,
                     NodeKind::Oscillator, param, value});
  }

  void render(NodeHandle destination, int frames, float sampleRate,
              std::vector<float> &out) override {
    (void)destination;
    (void)sampleRate;
    out.assign(frames < 0 ? 0 : static_cast<std::size_t>(frames), 0.0f);
  }

  void renderStereo(NodeHandle destination, int frames, float sampleRate,
                    std::vector<float> &outLR) override {
    (void)destination;
    (void)sampleRate;
    // Silence — no DSP in the recording tier.
    outLR.assign(frames < 0 ? 0 : static_cast<std::size_t>(frames) * 2, 0.0f);
  }

  std::vector<CreateOp> creates;
  std::vector<ConnectOp> connects;
  std::vector<SetParamOp> setParams;
  std::vector<Op> ops;  ///< all ops in receive order (op-sequence determinism)

private:
  NodeHandle lastHandle_ = kInvalidNodeHandle;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_RECORDING_BACKEND_HPP
