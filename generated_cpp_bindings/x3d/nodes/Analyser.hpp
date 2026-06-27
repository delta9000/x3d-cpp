// Analyser.hpp
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
 * @class Analyser
 * @brief Analyser provides real-time frequency and time-domain analysis
 * information, without any change to the input.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Analyser
 */
class Analyser : public virtual X3DSoundProcessingNode {
public:
  /**
   * @brief Default constructor for Analyser
   */
  Analyser() = default;

  /**
   * @brief Destructor for Analyser
   */
  ~Analyser() = default;

  /**
   * @brief Get the default value for fftSize
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFftSize() { return 2048; }

  /**
   * @brief Get the default value for frequencyBinCount
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFrequencyBinCount() { return 1024; }

  /**
   * @brief Get the default value for maxDecibels
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxDecibels() { return -30; }

  /**
   * @brief Get the default value for minDecibels
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinDecibels() { return -100; }

  /**
   * @brief Get the default value for smoothingTimeConstant
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSmoothingTimeConstant() { return 0.8; }

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
   * @brief Gets the value of fftSize. AccessType: inputOutput
   * @details fftSize represents size of Fast Fourier Transform (FFT) used to
   * determine frequency domain.
   * @return SFInt32 The current value of fftSize.
   */
  SFInt32 getFftSize() const { return _fftSize; }

  /**
   * @brief Sets the value of fftSize. AccessType: inputOutput
   * @details fftSize represents size of Fast Fourier Transform (FFT) used to
   * determine frequency domain.
   * @param value The new value for fftSize.
   */
  void setFftSize(const SFInt32 &value) {

    validateFftSize(value);

    _fftSize = value;
  }

  /**
   * @brief Non-validating write of fftSize (runtime/reader ingest path).
   * @details Assigns without the range check setFftSize() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFftSize() stays the
   *          enforcement point for programmatic callers.
   */
  void setFftSizeUnchecked(const SFInt32 &value) { _fftSize = value; }

  /**
   * @brief Gets the value of frequencyBinCount. AccessType: inputOutput
   * @details frequencyBinCount is half of fftSize and generally equates to
   * number of data values available for the visualization.
   * @return SFInt32 The current value of frequencyBinCount.
   */
  SFInt32 getFrequencyBinCount() const { return _frequencyBinCount; }

  /**
   * @brief Sets the value of frequencyBinCount. AccessType: inputOutput
   * @details frequencyBinCount is half of fftSize and generally equates to
   * number of data values available for the visualization.
   * @param value The new value for frequencyBinCount.
   */
  void setFrequencyBinCount(const SFInt32 &value) {

    validateFrequencyBinCount(value);

    _frequencyBinCount = value;
  }

  /**
   * @brief Non-validating write of frequencyBinCount (runtime/reader ingest
   * path).
   * @details Assigns without the range check setFrequencyBinCount() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFrequencyBinCount() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrequencyBinCountUnchecked(const SFInt32 &value) {
    _frequencyBinCount = value;
  }

  /**
   * @brief Gets the value of maxDecibels. AccessType: inputOutput
   * @details maxDecibels represents maximum power value in scaling range for
   * FFT analysis data.
   * @return SFFloat The current value of maxDecibels.
   */
  SFFloat getMaxDecibels() const { return _maxDecibels; }

  /**
   * @brief Sets the value of maxDecibels. AccessType: inputOutput
   * @details maxDecibels represents maximum power value in scaling range for
   * FFT analysis data.
   * @param value The new value for maxDecibels.
   */
  void setMaxDecibels(const SFFloat &value) { _maxDecibels = value; }

  /**
   * @brief Gets the value of minDecibels. AccessType: inputOutput
   * @details minDecibels represents minimum power value in scaling range for
   * FFT analysis data.
   * @return SFFloat The current value of minDecibels.
   */
  SFFloat getMinDecibels() const { return _minDecibels; }

  /**
   * @brief Sets the value of minDecibels. AccessType: inputOutput
   * @details minDecibels represents minimum power value in scaling range for
   * FFT analysis data.
   * @param value The new value for minDecibels.
   */
  void setMinDecibels(const SFFloat &value) { _minDecibels = value; }

  /**
   * @brief Gets the value of smoothingTimeConstant. AccessType: inputOutput
   * @details smoothingTimeConstant represents averaging constant during last
   * analysis frame.
   * @return SFFloat The current value of smoothingTimeConstant.
   */
  SFFloat getSmoothingTimeConstant() const { return _smoothingTimeConstant; }

  /**
   * @brief Sets the value of smoothingTimeConstant. AccessType: inputOutput
   * @details smoothingTimeConstant represents averaging constant during last
   * analysis frame.
   * @param value The new value for smoothingTimeConstant.
   */
  void setSmoothingTimeConstant(const SFFloat &value) {

    validateSmoothingTimeConstant(value);

    _smoothingTimeConstant = value;
  }

  /**
   * @brief Non-validating write of smoothingTimeConstant (runtime/reader ingest
   * path).
   * @details Assigns without the range check setSmoothingTimeConstant()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setSmoothingTimeConstant() stays the
   *          enforcement point for programmatic callers.
   */
  void setSmoothingTimeConstantUnchecked(const SFFloat &value) {
    _smoothingTimeConstant = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "Analyser").
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
  static void checkRangesFftSize(const SFInt32 &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrequencyBinCount(const SFInt32 &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSmoothingTimeConstant(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

private:
  static void validateFftSize(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("fftSize below minimum of 0");
  }

  static void validateFrequencyBinCount(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("frequencyBinCount below minimum of 0");
  }

  static void validateSmoothingTimeConstant(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("smoothingTimeConstant below minimum of 0");
  }

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for fftSize.
   */

  SFInt32 _fftSize{2048};

  /**
   * @brief Member variable for frequencyBinCount.
   */

  SFInt32 _frequencyBinCount{1024};

  /**
   * @brief Member variable for maxDecibels.
   */

  SFFloat _maxDecibels{-30};

  /**
   * @brief Member variable for minDecibels.
   */

  SFFloat _minDecibels{-100};

  /**
   * @brief Member variable for smoothingTimeConstant.
   */

  SFFloat _smoothingTimeConstant{0.8};
};

} // namespace x3d::nodes
