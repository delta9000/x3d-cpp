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
  Analyser() = default;

  ~Analyser() = default;

  static SFInt32 getDefaultFftSize() { return 2048; }

  static SFInt32 getDefaultFrequencyBinCount() { return 1024; }

  static SFFloat getDefaultMaxDecibels() { return -30; }

  static SFFloat getDefaultMinDecibels() { return -100; }

  static SFFloat getDefaultSmoothingTimeConstant() { return 0.8; }

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
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details fftSize represents size of Fast Fourier Transform (FFT) used to
   * determine frequency domain.
   */
  const SFInt32 &getFftSize() const { return _fftSize; }

  /**
   * Access: inputOutput.
   * @details fftSize represents size of Fast Fourier Transform (FFT) used to
   * determine frequency domain.
   */
  void setFftSize(const SFInt32 &value) {

    validateFftSize(value);

    _fftSize = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFftSize() for range validation.
   */
  void setFftSizeUnchecked(const SFInt32 &value) { _fftSize = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details frequencyBinCount is half of fftSize and generally equates to
   * number of data values available for the visualization.
   */
  const SFInt32 &getFrequencyBinCount() const { return _frequencyBinCount; }

  /**
   * Access: inputOutput.
   * @details frequencyBinCount is half of fftSize and generally equates to
   * number of data values available for the visualization.
   */
  void setFrequencyBinCount(const SFInt32 &value) {

    validateFrequencyBinCount(value);

    _frequencyBinCount = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrequencyBinCount() for range validation.
   */
  void setFrequencyBinCountUnchecked(const SFInt32 &value) {
    _frequencyBinCount = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxDecibels represents maximum power value in scaling range for
   * FFT analysis data.
   */
  const SFFloat &getMaxDecibels() const { return _maxDecibels; }

  /**
   * Access: inputOutput.
   * @details maxDecibels represents maximum power value in scaling range for
   * FFT analysis data.
   */
  void setMaxDecibels(const SFFloat &value) { _maxDecibels = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minDecibels represents minimum power value in scaling range for
   * FFT analysis data.
   */
  const SFFloat &getMinDecibels() const { return _minDecibels; }

  /**
   * Access: inputOutput.
   * @details minDecibels represents minimum power value in scaling range for
   * FFT analysis data.
   */
  void setMinDecibels(const SFFloat &value) { _minDecibels = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details smoothingTimeConstant represents averaging constant during last
   * analysis frame.
   */
  const SFFloat &getSmoothingTimeConstant() const {
    return _smoothingTimeConstant;
  }

  /**
   * Access: inputOutput.
   * @details smoothingTimeConstant represents averaging constant during last
   * analysis frame.
   */
  void setSmoothingTimeConstant(const SFFloat &value) {

    validateSmoothingTimeConstant(value);

    _smoothingTimeConstant = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSmoothingTimeConstant() for range validation.
   */
  void setSmoothingTimeConstantUnchecked(const SFFloat &value) {
    _smoothingTimeConstant = value;
  }

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

    if (value < 0.0f)
      throw std::out_of_range("smoothingTimeConstant below minimum of 0");
  }

  MFNode _children{};

  SFInt32 _fftSize{2048};

  SFInt32 _frequencyBinCount{1024};

  SFFloat _maxDecibels{-30};

  SFFloat _minDecibels{-100};

  SFFloat _smoothingTimeConstant{0.8};
};

} // namespace x3d::nodes
