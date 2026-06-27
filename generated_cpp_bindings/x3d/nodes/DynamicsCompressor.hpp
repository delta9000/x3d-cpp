// DynamicsCompressor.hpp
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
  /**
   * @brief Default constructor for DynamicsCompressor
   */
  DynamicsCompressor() = default;

  /**
   * @brief Destructor for DynamicsCompressor
   */
  ~DynamicsCompressor() = default;

  /**
   * @brief Get the default value for attack
   * @return SFTime The default value
   */
  static SFTime getDefaultAttack() { return 0.003; }

  /**
   * @brief Get the default value for knee
   * @return SFFloat The default value
   */
  static SFFloat getDefaultKnee() { return 30; }

  /**
   * @brief Get the default value for ratio
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRatio() { return 12; }

  /**
   * @brief Get the default value for release
   * @return SFTime The default value
   */
  static SFTime getDefaultRelease() { return 0.25; }

  /**
   * @brief Get the default value for threshold
   * @return SFFloat The default value
   */
  static SFFloat getDefaultThreshold() { return -24; }

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
   * @brief Gets the value of attack. AccessType: inputOutput
   * @details The attack field is the duration of time (in seconds) to reduce
   * the gain by 10dB.
   * @return SFTime The current value of attack.
   */
  SFTime getAttack() const { return _attack; }

  /**
   * @brief Sets the value of attack. AccessType: inputOutput
   * @details The attack field is the duration of time (in seconds) to reduce
   * the gain by 10dB.
   * @param value The new value for attack.
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
   * @brief Non-validating write of attack (runtime/reader ingest path).
   * @details Assigns without the range check setAttack() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAttack() stays the
   *          enforcement point for programmatic callers.
   */
  void setAttackUnchecked(const SFTime &value) { _attack = value; }

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
   * @brief Gets the value of knee. AccessType: inputOutput
   * @details knee field contains a decibel value representing range above
   * threshold where the curve smoothly transitions to compressed portion.
   * @return SFFloat The current value of knee.
   */
  SFFloat getKnee() const { return _knee; }

  /**
   * @brief Sets the value of knee. AccessType: inputOutput
   * @details knee field contains a decibel value representing range above
   * threshold where the curve smoothly transitions to compressed portion.
   * @param value The new value for knee.
   */
  void setKnee(const SFFloat &value) {

    validateKnee(value);

    _knee = value;
  }

  /**
   * @brief Non-validating write of knee (runtime/reader ingest path).
   * @details Assigns without the range check setKnee() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setKnee() stays the
   *          enforcement point for programmatic callers.
   */
  void setKneeUnchecked(const SFFloat &value) { _knee = value; }

  /**
   * @brief Gets the value of ratio. AccessType: inputOutput
   * @details ratio field represents amount of input change, in dB, needed for 1
   * dB change in output.
   * @return SFFloat The current value of ratio.
   */
  SFFloat getRatio() const { return _ratio; }

  /**
   * @brief Sets the value of ratio. AccessType: inputOutput
   * @details ratio field represents amount of input change, in dB, needed for 1
   * dB change in output.
   * @param value The new value for ratio.
   */
  void setRatio(const SFFloat &value) {

    validateRatio(value);

    _ratio = value;
  }

  /**
   * @brief Non-validating write of ratio (runtime/reader ingest path).
   * @details Assigns without the range check setRatio() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRatio() stays the
   *          enforcement point for programmatic callers.
   */
  void setRatioUnchecked(const SFFloat &value) { _ratio = value; }

  /**
   * @brief Gets the value of reduction. AccessType: outputOnly
   * @details reduction field provides amount of gain reduction in dB currently
   * applied by compressor to signal.
   * @return SFFloat The current value of reduction.
   */
  SFFloat getReduction() const { return _reduction; }

  /**
   * @brief Emit an output value on reduction. AccessType: outputOnly
   * @details reduction field provides amount of gain reduction in dB currently
   * applied by compressor to signal. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitReduction(const SFFloat &value) { _reduction = value; }

  /**
   * @brief Gets the value of release. AccessType: inputOutput
   * @details release field represents amount of time (in seconds) to increase
   * gain by 10dB.
   * @return SFTime The current value of release.
   */
  SFTime getRelease() const { return _release; }

  /**
   * @brief Sets the value of release. AccessType: inputOutput
   * @details release field represents amount of time (in seconds) to increase
   * gain by 10dB.
   * @param value The new value for release.
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
   * @brief Non-validating write of release (runtime/reader ingest path).
   * @details Assigns without the range check setRelease() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRelease() stays the
   *          enforcement point for programmatic callers.
   */
  void setReleaseUnchecked(const SFTime &value) { _release = value; }

  /**
   * @brief Gets the value of threshold. AccessType: inputOutput
   * @details threshold field represents decibel value above which compression
   * starts taking effect.
   * @return SFFloat The current value of threshold.
   */
  SFFloat getThreshold() const { return _threshold; }

  /**
   * @brief Sets the value of threshold. AccessType: inputOutput
   * @details threshold field represents decibel value above which compression
   * starts taking effect.
   * @param value The new value for threshold.
   */
  void setThreshold(const SFFloat &value) {

    validateThreshold(value);

    _threshold = value;
  }

  /**
   * @brief Non-validating write of threshold (runtime/reader ingest path).
   * @details Assigns without the range check setThreshold() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setThreshold() stays the
   *          enforcement point for programmatic callers.
   */
  void setThresholdUnchecked(const SFFloat &value) { _threshold = value; }

  /**
   * @brief The X3D type name of this node (e.g. "DynamicsCompressor").
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

    if (value < 0)
      throw std::out_of_range("knee below minimum of 0");
  }

  static void validateRatio(const SFFloat &value) {

    if (value < 1)
      throw std::out_of_range("ratio below minimum of 1");
    if (value > 20)
      throw std::out_of_range("ratio above maximum of 20");
  }

  static void validateRelease(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("release below minimum of 0");
  }

  static void validateThreshold(const SFFloat &value) {

    if (value < -100)
      throw std::out_of_range("threshold below minimum of -100");
    if (value > 0)
      throw std::out_of_range("threshold above maximum of 0");
  }

  /**
   * @brief Member variable for attack.
   */

  SFTime _attack{0.003};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for knee.
   */

  SFFloat _knee{30};

  /**
   * @brief Member variable for ratio.
   */

  SFFloat _ratio{12};

  /**
   * @brief Member variable for reduction.
   */

  SFFloat _reduction{};

  /**
   * @brief Member variable for release.
   */

  SFTime _release{0.25};

  /**
   * @brief Member variable for threshold.
   */

  SFFloat _threshold{-24};
};

} // namespace x3d::nodes
