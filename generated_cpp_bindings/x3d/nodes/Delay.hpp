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
 * @class Delay
 * @brief Delay causes a time delay between the arrival of input data and
 * subsequent propagation to the output.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Delay
 */
class Delay : public virtual X3DSoundProcessingNode {
public:
  Delay() = default;

  ~Delay() = default;

  static SFTime getDefaultDelayTime() { return 0; }

  static SFTime getDefaultMaxDelayTime() { return 1; }

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
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details delayTime is duration of delay (in seconds) to apply.
   */
  const SFTime &getDelayTime() const { return _delayTime; }

  /**
   * Access: inputOutput.
   * @details delayTime is duration of delay (in seconds) to apply.
   */
  void setDelayTime(const SFTime &value) {

    validateDelayTime(value);

    _delayTime = value;
  }

  void setDelayTime(SFTime &&value) {

    validateDelayTime(value);

    _delayTime = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDelayTime() for range validation.
   */
  void setDelayTimeUnchecked(const SFTime &value) { _delayTime = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details maxDelayTime is duration of maximum amount of delay (in seconds)
   * that can be applied.
   */
  const SFTime &getMaxDelayTime() const { return _maxDelayTime; }

  /**
   * Access: inputOutput.
   * @details maxDelayTime is duration of maximum amount of delay (in seconds)
   * that can be applied.
   */
  void setMaxDelayTime(const SFTime &value) {

    validateMaxDelayTime(value);

    _maxDelayTime = value;
  }

  void setMaxDelayTime(SFTime &&value) {

    validateMaxDelayTime(value);

    _maxDelayTime = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxDelayTime() for range validation.
   */
  void setMaxDelayTimeUnchecked(const SFTime &value) { _maxDelayTime = value; }

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
  static void checkRangesDelayTime(const SFTime &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxDelayTime(const SFTime &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

private:
  static void validateDelayTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("delayTime below minimum of 0");
  }

  static void validateMaxDelayTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("maxDelayTime below minimum of 0");
  }

  MFNode _children{};

  SFTime _delayTime{0};

  SFTime _maxDelayTime{1};
};

} // namespace x3d::nodes
