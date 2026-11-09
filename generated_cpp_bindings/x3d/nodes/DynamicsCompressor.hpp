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
 * @class DynamicsCompressor
 * @brief DynamicsCompressor node implements a dynamics compression effect,
 * lowering volume of loudest parts of signal and raising volume of softest
 * parts.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#DynamicsCompressor
 */
class DynamicsCompressor : public virtual X3DSoundProcessingNode {
public:
  DynamicsCompressor() = default;

  ~DynamicsCompressor() = default;

  static SFTime getDefaultAttack() { return 0.003; }

  static SFFloat getDefaultKnee() { return 30; }

  static SFFloat getDefaultRatio() { return 12; }

  static SFTime getDefaultRelease() { return 0.25; }

  static SFFloat getDefaultThreshold() { return -24; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details The attack field is the duration of time (in seconds) to reduce
   * the gain by 10dB.
   */
  const SFTime &getAttack() const { return _attack; }

  /**
   * Access: inputOutput.
   * @details The attack field is the duration of time (in seconds) to reduce
   * the gain by 10dB.
   */
  void setAttack(const SFTime &value) {

    validateAttack(value);

    _attack = value;
  }

  void setAttack(SFTime &&value) {

    validateAttack(value);

    _attack = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAttack() for range validation.
   */
  void setAttackUnchecked(const SFTime &value) { _attack = value; }

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
   * @details knee field contains a decibel value representing range above
   * threshold where the curve smoothly transitions to compressed portion.
   */
  const SFFloat &getKnee() const { return _knee; }

  /**
   * Access: inputOutput.
   * @details knee field contains a decibel value representing range above
   * threshold where the curve smoothly transitions to compressed portion.
   */
  void setKnee(const SFFloat &value) {

    validateKnee(value);

    _knee = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setKnee() for range validation.
   */
  void setKneeUnchecked(const SFFloat &value) { _knee = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details ratio field represents amount of input change, in dB, needed for 1
   * dB change in output.
   */
  const SFFloat &getRatio() const { return _ratio; }

  /**
   * Access: inputOutput.
   * @details ratio field represents amount of input change, in dB, needed for 1
   * dB change in output.
   */
  void setRatio(const SFFloat &value) {

    validateRatio(value);

    _ratio = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRatio() for range validation.
   */
  void setRatioUnchecked(const SFFloat &value) { _ratio = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details reduction field provides amount of gain reduction in dB currently
   * applied by compressor to signal.
   */
  const SFFloat &getReduction() const { return _reduction; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details reduction field provides amount of gain reduction in dB currently
   * applied by compressor to signal.
   */
  void emitReduction(const SFFloat &value) { _reduction = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details release field represents amount of time (in seconds) to increase
   * gain by 10dB.
   */
  const SFTime &getRelease() const { return _release; }

  /**
   * Access: inputOutput.
   * @details release field represents amount of time (in seconds) to increase
   * gain by 10dB.
   */
  void setRelease(const SFTime &value) {

    validateRelease(value);

    _release = value;
  }

  void setRelease(SFTime &&value) {

    validateRelease(value);

    _release = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRelease() for range validation.
   */
  void setReleaseUnchecked(const SFTime &value) { _release = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details threshold field represents decibel value above which compression
   * starts taking effect.
   */
  const SFFloat &getThreshold() const { return _threshold; }

  /**
   * Access: inputOutput.
   * @details threshold field represents decibel value above which compression
   * starts taking effect.
   */
  void setThreshold(const SFFloat &value) {

    validateThreshold(value);

    _threshold = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setThreshold() for range validation.
   */
  void setThresholdUnchecked(const SFFloat &value) { _threshold = value; }

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
  static void checkRangesAttack(const SFTime &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesKnee(const SFFloat &value, const std::string &nodeType,
                              const std::string &defName,
                              std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRatio(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRelease(const SFTime &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesThreshold(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateAttack(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("attack below minimum of 0");
  }

  static void validateKnee(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("knee below minimum of 0");
  }

  static void validateRatio(const SFFloat &value) {

    if (value < 1.0f)
      throw std::out_of_range("ratio below minimum of 1");
    if (value > 20.0f)
      throw std::out_of_range("ratio above maximum of 20");
  }

  static void validateRelease(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("release below minimum of 0");
  }

  static void validateThreshold(const SFFloat &value) {

    if (value < -100.0f)
      throw std::out_of_range("threshold below minimum of -100");
    if (value > 0.0f)
      throw std::out_of_range("threshold above maximum of 0");
  }

  SFTime _attack{0.003};

  MFNode _children{};

  SFFloat _knee{30};

  SFFloat _ratio{12};

  SFFloat _reduction{};

  SFTime _release{0.25};

  SFFloat _threshold{-24};
};

} // namespace x3d::nodes
