// AudioDestination.hpp
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
 * @class AudioDestination
 * @brief AudioDestination node represents the final audio destination and is
 * what user ultimately hears, typically from the speakers of user device.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#AudioDestination
 */
class AudioDestination : public virtual X3DSoundDestinationNode {
public:
  /**
   * @brief Default constructor for AudioDestination
   */
  AudioDestination() = default;

  /**
   * @brief Destructor for AudioDestination
   */
  ~AudioDestination() = default;

  /**
   * @brief Get the default value for maxChannelCount
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMaxChannelCount() { return 2; }

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
   * @brief Gets the value of maxChannelCount. AccessType: inputOutput
   * @details [maxChannelCount.
   * @return SFInt32 The current value of maxChannelCount.
   */
  SFInt32 getMaxChannelCount() const { return _maxChannelCount; }

  /**
   * @brief Sets the value of maxChannelCount. AccessType: inputOutput
   * @details [maxChannelCount.
   * @param value The new value for maxChannelCount.
   */
  void setMaxChannelCount(const SFInt32 &value) {

    validateMaxChannelCount(value);

    _maxChannelCount = value;
  }

  /**
   * @brief Non-validating write of maxChannelCount (runtime/reader ingest
   * path).
   * @details Assigns without the range check setMaxChannelCount() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setMaxChannelCount() stays the enforcement point for
   * programmatic callers.
   */
  void setMaxChannelCountUnchecked(const SFInt32 &value) {
    _maxChannelCount = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "AudioDestination").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxChannelCount(const SFInt32 &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateMaxChannelCount(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("maxChannelCount below minimum of 0");
  }

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for maxChannelCount.
   */

  SFInt32 _maxChannelCount{2};
};

} // namespace x3d::nodes
