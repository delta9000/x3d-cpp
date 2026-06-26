// Delay.hpp
#ifndef DELAY_HPP
#define DELAY_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DTimeDependentNode.hpp"

#include "X3DSoundNode.hpp"

#include "X3DSoundProcessingNode.hpp"

/**
 * @class Delay
 * @brief Delay causes a time delay between the arrival of input data and
 * subsequent propagation to the output.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Delay
 */
class Delay : public virtual X3DSoundProcessingNode {
public:
  /**
   * @brief Default constructor for Delay
   */
  Delay() = default;

  /**
   * @brief Destructor for Delay
   */
  ~Delay() = default;

  /**
   * @brief Get the default value for delayTime
   * @return SFTime The default value
   */
  static SFTime getDefaultDelayTime() { return 0; }

  /**
   * @brief Get the default value for maxDelayTime
   * @return SFTime The default value
   */
  static SFTime getDefaultMaxDelayTime() { return 1; }

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
   * @brief Gets the value of delayTime. AccessType: inputOutput
   * @details delayTime is duration of delay (in seconds) to apply.
   * @return SFTime The current value of delayTime.
   */
  SFTime getDelayTime() const { return _delayTime; }

  /**
   * @brief Sets the value of delayTime. AccessType: inputOutput
   * @details delayTime is duration of delay (in seconds) to apply.
   * @param value The new value for delayTime.
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
   * @brief Non-validating write of delayTime (runtime/reader ingest path).
   * @details Assigns without the range check setDelayTime() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDelayTime() stays the
   *          enforcement point for programmatic callers.
   */
  void setDelayTimeUnchecked(const SFTime &value) { _delayTime = value; }

  /**
   * @brief Gets the value of maxDelayTime. AccessType: inputOutput
   * @details maxDelayTime is duration of maximum amount of delay (in seconds)
   * that can be applied.
   * @return SFTime The current value of maxDelayTime.
   */
  SFTime getMaxDelayTime() const { return _maxDelayTime; }

  /**
   * @brief Sets the value of maxDelayTime. AccessType: inputOutput
   * @details maxDelayTime is duration of maximum amount of delay (in seconds)
   * that can be applied.
   * @param value The new value for maxDelayTime.
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
   * @brief Non-validating write of maxDelayTime (runtime/reader ingest path).
   * @details Assigns without the range check setMaxDelayTime() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxDelayTime() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxDelayTimeUnchecked(const SFTime &value) { _maxDelayTime = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Delay").
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

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for delayTime.
   */

  SFTime _delayTime{0};

  /**
   * @brief Member variable for maxDelayTime.
   */

  SFTime _maxDelayTime{1};
};

#endif // DELAY_HPP