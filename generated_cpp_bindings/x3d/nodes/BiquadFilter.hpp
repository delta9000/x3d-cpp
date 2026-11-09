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
  BiquadFilter() = default;

  ~BiquadFilter() = default;

  static SFFloat getDefaultDetune() { return 0; }

  static SFFloat getDefaultFrequency() { return 350; }

  static SFFloat getDefaultQualityFactor() { return 1; }

  static BiquadTypeFilterChoices getDefaultType() {
    return BiquadTypeFilterChoices::LOWPASS;
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node.
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
   * input signals for this node.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The detune field forms a compound field together with playbackRate
   * that together determine a computedPlaybackRate value.
   */
  const SFFloat &getDetune() const { return _detune; }

  /**
   * Access: inputOutput.
   * @details The detune field forms a compound field together with playbackRate
   * that together determine a computedPlaybackRate value.
   */
  void setDetune(const SFFloat &value) {

    validateDetune(value);

    _detune = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDetune() for range validation.
   */
  void setDetuneUnchecked(const SFFloat &value) { _detune = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details frequency at which the BiquadFilterNode operates, in Hz.
   */
  const SFFloat &getFrequency() const { return _frequency; }

  /**
   * Access: inputOutput.
   * @details frequency at which the BiquadFilterNode operates, in Hz.
   */
  void setFrequency(const SFFloat &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrequency() for range validation.
   */
  void setFrequencyUnchecked(const SFFloat &value) { _frequency = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details qualityFactor is Quality Factor (Q) of the respective filter
   * algorithm.
   */
  const SFFloat &getQualityFactor() const { return _qualityFactor; }

  /**
   * Access: inputOutput.
   * @details qualityFactor is Quality Factor (Q) of the respective filter
   * algorithm.
   */
  void setQualityFactor(const SFFloat &value) {

    validateQualityFactor(value);

    _qualityFactor = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setQualityFactor() for range validation.
   */
  void setQualityFactorUnchecked(const SFFloat &value) {
    _qualityFactor = value;
  }

  /**
   * BiquadTypeFilterChoices; access: inputOutput. Borrows the stored value.
   * @details type selects which BiquadFilter algorithm is used.
   */
  const BiquadTypeFilterChoices &getType() const { return _type; }

  /**
   * Access: inputOutput.
   * @details type selects which BiquadFilter algorithm is used.
   */
  void setType(const BiquadTypeFilterChoices &value) { _type = value; }

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

    if (value < 0.0f)
      throw std::out_of_range("detune below minimum of 0");
  }

  static void validateFrequency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("frequency below minimum of 0");
  }

  static void validateQualityFactor(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("qualityFactor below minimum of 0");
  }

  MFNode _children{};

  SFFloat _detune{0};

  SFFloat _frequency{350};

  SFFloat _qualityFactor{1};

  BiquadTypeFilterChoices _type{BiquadTypeFilterChoices::LOWPASS};
};

} // namespace x3d::nodes
