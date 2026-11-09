#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DSoundNode.hpp"

#include "x3d/nodes/X3DSoundChannelNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ChannelSplitter
 * @brief ChannelSplitter separates the different channels of a single audio
 * source into a set of monophonic output channels.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#ChannelSplitter
 */
class ChannelSplitter : public virtual X3DSoundChannelNode {
public:
  ChannelSplitter() = default;

  ~ChannelSplitter() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node, making up a section of the audio graph.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: Analyser, AudioClip, AudioDestination,
   * BiquadFilter, BufferAudioSource, ChannelMerger, ChannelSelector,
   * ChannelSplitter, Convolver, Delay, DynamicsCompressor, Gain,
   * ListenerPointSource, MicrophoneSource, MovieTexture, OscillatorSource,
   * Sound, SpatialSound, StreamAudioDestination, StreamAudioSource, WaveShaper
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"Analyser",
                                                   "AudioClip",
                                                   "AudioDestination",
                                                   "BiquadFilter",
                                                   "BufferAudioSource",
                                                   "ChannelMerger",
                                                   "ChannelSelector",
                                                   "ChannelSplitter",
                                                   "Convolver",
                                                   "Delay",
                                                   "DynamicsCompressor",
                                                   "Gain",
                                                   "ListenerPointSource",
                                                   "MicrophoneSource",
                                                   "MovieTexture",
                                                   "OscillatorSource",
                                                   "Sound",
                                                   "SpatialSound",
                                                   "StreamAudioDestination",
                                                   "StreamAudioSource",
                                                   "WaveShaper"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node, making up a section of the audio graph.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The outputs field is a set of output nodes receiving the split
   * channels, and making up a section of the audio graph.
   */
  const MFNode &getOutputs() const { return _outputs; }

  /**
   * @details Permitted X3D node types: X3DSoundChannelNode,
   * X3DSoundProcessingNode, X3DSoundSourceNode
   */
  static const std::vector<std::string> &acceptableOutputsNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSoundChannelNode", "X3DSoundProcessingNode", "X3DSoundSourceNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The outputs field is a set of output nodes receiving the split
   * channels, and making up a section of the audio graph.
   */
  void setOutputs(const MFNode &value) { _outputs = value; }

  void setOutputs(MFNode &&value) { _outputs = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  MFNode _children{};

  MFNode _outputs{};
};

} // namespace x3d::nodes
