// ChannelSplitter.hpp
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
  /**
   * @brief Default constructor for ChannelSplitter
   */
  ChannelSplitter() = default;

  /**
   * @brief Destructor for ChannelSplitter
   */
  ~ChannelSplitter() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Sound"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node, making up a section of the audio graph.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: Analyser, AudioClip, AudioDestination,
   * BiquadFilter, BufferAudioSource, ChannelMerger, ChannelSelector,
   * ChannelSplitter, Convolver, Delay, DynamicsCompressor, Gain,
   * ListenerPointSource, MicrophoneSource, MovieTexture, OscillatorSource,
   * Sound, SpatialSound, StreamAudioDestination, StreamAudioSource, WaveShaper
   * @return const std::vector<std::string>& The accepted node type names.
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
   * @brief Sets the value of children. AccessType: inputOutput
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node, making up a section of the audio graph.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of outputs. AccessType: inputOutput
   * @details The outputs field is a set of output nodes receiving the split
   * channels, and making up a section of the audio graph.
   * @return MFNode The current value of outputs.
   */
  MFNode getOutputs() const { return _outputs; }

  /**
   * @brief Acceptable node types for the outputs field.
   * @details Permitted X3D node types: X3DSoundChannelNode,
   * X3DSoundProcessingNode, X3DSoundSourceNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableOutputsNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSoundChannelNode", "X3DSoundProcessingNode", "X3DSoundSourceNode"};
    return types;
  }

  /**
   * @brief Sets the value of outputs. AccessType: inputOutput
   * @details The outputs field is a set of output nodes receiving the split
   * channels, and making up a section of the audio graph.
   * @param value The new value for outputs.
   */
  void setOutputs(const MFNode &value) { _outputs = value; }

  void setOutputs(MFNode &&value) { _outputs = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ChannelSplitter").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

private:
  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for outputs.
   */

  MFNode _outputs{};
};

} // namespace x3d::nodes
