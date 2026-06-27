// StreamAudioDestination.hpp
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

#include "x3d/nodes/X3DSoundDestinationNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class StreamAudioDestination
 * @brief StreamAudioDestination node represents the final audio destination via
 * a media stream.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#StreamAudioDestination
 */
class StreamAudioDestination : public virtual X3DSoundDestinationNode {
public:
  /**
   * @brief Default constructor for StreamAudioDestination
   */
  StreamAudioDestination() = default;

  /**
   * @brief Destructor for StreamAudioDestination
   */
  ~StreamAudioDestination() = default;

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
   * input signals for this node.
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
   * input signals for this node.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of streamIdentifier. AccessType: inputOutput
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   * @return SFString The current value of streamIdentifier.
   */
  SFString getStreamIdentifier() const { return _streamIdentifier; }

  /**
   * @brief Sets the value of streamIdentifier. AccessType: inputOutput
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   * @param value The new value for streamIdentifier.
   */
  void setStreamIdentifier(const SFString &value) { _streamIdentifier = value; }

  void setStreamIdentifier(SFString &&value) {

    _streamIdentifier = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "StreamAudioDestination").
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
   * @brief Member variable for streamIdentifier.
   */

  SFString _streamIdentifier{};
};

} // namespace x3d::nodes
