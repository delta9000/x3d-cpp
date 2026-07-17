// BiquadFilter.hpp
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

#include "x3d/nodes/X3DTimeDependentNode.hpp"

#include "x3d/nodes/X3DSoundNode.hpp"

#include "x3d/nodes/X3DSoundProcessingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class BiquadFilter
 * @brief BiquadFilter node is an AudioNode processor implementing common
 * low-order filters.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#BiquadFilter
 */
class BiquadFilter : public virtual X3DSoundProcessingNode {
public:
  /**
   * @brief Default constructor for BiquadFilter
   */
  BiquadFilter() = default;

  /**
   * @brief Destructor for BiquadFilter
   */
  ~BiquadFilter() = default;

  /**
   * @brief Get the default value for detune
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDetune() { return 0; }

  /**
   * @brief Get the default value for frequency
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFrequency() { return 350; }

  /**
   * @brief Get the default value for qualityFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultQualityFactor() { return 1; }

  /**
   * @brief Get the default value for type
   * @return BiquadTypeFilterChoices The default value
   */
  static BiquadTypeFilterChoices getDefaultType() {
    return BiquadTypeFilterChoices::LOWPASS;
  }

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
   * @return const MFNode& The current value of children.
   */
  const MFNode &getChildren() const { return _children; }

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
   * @brief Gets the value of detune. AccessType: inputOutput
   * @details The detune field forms a compound field together with playbackRate
   * that together determine a computedPlaybackRate value.
   * @return SFFloat The current value of detune.
   */
  SFFloat getDetune() const { return _detune; }

  /**
   * @brief Sets the value of detune. AccessType: inputOutput
   * @details The detune field forms a compound field together with playbackRate
   * that together determine a computedPlaybackRate value.
   * @param value The new value for detune.
   */
  void setDetune(const SFFloat &value) {

    validateDetune(value);

    _detune = value;
  }

  /**
   * @brief Non-validating write of detune (runtime/reader ingest path).
   * @details Assigns without the range check setDetune() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDetune() stays the
   *          enforcement point for programmatic callers.
   */
  void setDetuneUnchecked(const SFFloat &value) { _detune = value; }

  /**
   * @brief Gets the value of frequency. AccessType: inputOutput
   * @details frequency at which the BiquadFilterNode operates, in Hz.
   * @return SFFloat The current value of frequency.
   */
  SFFloat getFrequency() const { return _frequency; }

  /**
   * @brief Sets the value of frequency. AccessType: inputOutput
   * @details frequency at which the BiquadFilterNode operates, in Hz.
   * @param value The new value for frequency.
   */
  void setFrequency(const SFFloat &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * @brief Non-validating write of frequency (runtime/reader ingest path).
   * @details Assigns without the range check setFrequency() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFrequency() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrequencyUnchecked(const SFFloat &value) { _frequency = value; }

  /**
   * @brief Gets the value of qualityFactor. AccessType: inputOutput
   * @details qualityFactor is Quality Factor (Q) of the respective filter
   * algorithm.
   * @return SFFloat The current value of qualityFactor.
   */
  SFFloat getQualityFactor() const { return _qualityFactor; }

  /**
   * @brief Sets the value of qualityFactor. AccessType: inputOutput
   * @details qualityFactor is Quality Factor (Q) of the respective filter
   * algorithm.
   * @param value The new value for qualityFactor.
   */
  void setQualityFactor(const SFFloat &value) {

    validateQualityFactor(value);

    _qualityFactor = value;
  }

  /**
   * @brief Non-validating write of qualityFactor (runtime/reader ingest path).
   * @details Assigns without the range check setQualityFactor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setQualityFactor() stays the
   *          enforcement point for programmatic callers.
   */
  void setQualityFactorUnchecked(const SFFloat &value) {
    _qualityFactor = value;
  }

  /**
   * @brief Gets the value of type. AccessType: inputOutput
   * @details type selects which BiquadFilter algorithm is used.
   * @return BiquadTypeFilterChoices The current value of type.
   */
  BiquadTypeFilterChoices getType() const { return _type; }

  /**
   * @brief Sets the value of type. AccessType: inputOutput
   * @details type selects which BiquadFilter algorithm is used.
   * @param value The new value for type.
   */
  void setType(const BiquadTypeFilterChoices &value) { _type = value; }

  /**
   * @brief The X3D type name of this node (e.g. "BiquadFilter").
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
  static void checkRangesDetune(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrequency(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesQualityFactor(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateDetune(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("detune below minimum of 0");
  }

  static void validateFrequency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("frequency below minimum of 0");
  }

  static void validateQualityFactor(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("qualityFactor below minimum of 0");
  }

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for detune.
   */

  SFFloat _detune{0};

  /**
   * @brief Member variable for frequency.
   */

  SFFloat _frequency{350};

  /**
   * @brief Member variable for qualityFactor.
   */

  SFFloat _qualityFactor{1};

  /**
   * @brief Member variable for type.
   */

  BiquadTypeFilterChoices _type{BiquadTypeFilterChoices::LOWPASS};
};

} // namespace x3d::nodes
