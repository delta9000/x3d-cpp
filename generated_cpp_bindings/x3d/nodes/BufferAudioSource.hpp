// BufferAudioSource.hpp
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

#include "x3d/nodes/X3DSoundSourceNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class BufferAudioSource
 * @brief BufferAudioSource node represents a memory-resident audio asset that
 * can contain one or more channels.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#BufferAudioSource
 */
class BufferAudioSource : public virtual X3DSoundSourceNode,
                          public virtual X3DUrlObject {
public:
  /**
   * @brief Default constructor for BufferAudioSource
   */
  BufferAudioSource() = default;

  /**
   * @brief Destructor for BufferAudioSource
   */
  ~BufferAudioSource() = default;

  /**
   * @brief Get the default value for bufferDuration
   * @return SFTime The default value
   */
  static SFTime getDefaultBufferDuration() { return 0; }

  /**
   * @brief Get the default value for channelCountMode
   * @return ChannelCountModeChoices The default value
   */
  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  /**
   * @brief Get the default value for channelInterpretation
   * @return ChannelInterpretationChoices The default value
   */
  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  /**
   * @brief Get the default value for detune
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDetune() { return 0; }

  /**
   * @brief Get the default value for loop
   * @return SFBool The default value
   */
  static SFBool getDefaultLoop() { return false; }

  /**
   * @brief Get the default value for loopEnd
   * @return SFFloat The default value
   */
  static SFFloat getDefaultLoopEnd() { return 0; }

  /**
   * @brief Get the default value for loopStart
   * @return SFFloat The default value
   */
  static SFFloat getDefaultLoopStart() { return 0; }

  /**
   * @brief Get the default value for numberOfChannels
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultNumberOfChannels() { return 0; }

  /**
   * @brief Get the default value for playbackRate
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPlaybackRate() { return 1; }

  /**
   * @brief Get the default value for sampleRate
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSampleRate() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
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
   * @brief Gets the value of buffer. AccessType: inputOutput
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   * @return MFFloat The current value of buffer.
   */
  MFFloat getBuffer() const { return _buffer; }

  /**
   * @brief Sets the value of buffer. AccessType: inputOutput
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   * @param value The new value for buffer.
   */
  void setBuffer(const MFFloat &value) {

    validateBuffer(value);

    _buffer = value;
  }

  void setBuffer(MFFloat &&value) {

    validateBuffer(value);

    _buffer = std::move(value);
  }

  /**
   * @brief Non-validating write of buffer (runtime/reader ingest path).
   * @details Assigns without the range check setBuffer() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBuffer() stays the
   *          enforcement point for programmatic callers.
   */
  void setBufferUnchecked(const MFFloat &value) { _buffer = value; }

  /**
   * @brief Gets the value of bufferDuration. AccessType: inputOutput
   * @details bufferDuration is duration in seconds to use from buffer field.
   * @return SFTime The current value of bufferDuration.
   */
  SFTime getBufferDuration() const { return _bufferDuration; }

  /**
   * @brief Sets the value of bufferDuration. AccessType: inputOutput
   * @details bufferDuration is duration in seconds to use from buffer field.
   * @param value The new value for bufferDuration.
   */
  void setBufferDuration(const SFTime &value) {

    validateBufferDuration(value);

    _bufferDuration = value;
  }

  void setBufferDuration(SFTime &&value) {

    validateBufferDuration(value);

    _bufferDuration = std::move(value);
  }

  /**
   * @brief Non-validating write of bufferDuration (runtime/reader ingest path).
   * @details Assigns without the range check setBufferDuration() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBufferDuration() stays the
   *          enforcement point for programmatic callers.
   */
  void setBufferDurationUnchecked(const SFTime &value) {
    _bufferDuration = value;
  }

  /**
   * @brief Gets the value of bufferlength. AccessType: outputOnly
   * @details
   * @return SFInt32 The current value of bufferlength.
   */
  SFInt32 getBufferlength() const { return _bufferlength; }

  /**
   * @brief Emit an output value on bufferlength. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBufferlength(const SFInt32 &value) { _bufferlength = value; }

  /**
   * @brief Gets the value of channelCount. AccessType: outputOnly
   * @details channelCount reports number of channels provided by input nodes.
   * @return SFInt32 The current value of channelCount.
   */
  SFInt32 getChannelCount() const { return _channelCount; }

  /**
   * @brief Emit an output value on channelCount. AccessType: outputOnly
   * @details channelCount reports number of channels provided by input nodes.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitChannelCount(const SFInt32 &value) { _channelCount = value; }

  /**
   * @brief Gets the value of channelCountMode. AccessType: inputOutput
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   * @return ChannelCountModeChoices The current value of channelCountMode.
   */
  ChannelCountModeChoices getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * @brief Sets the value of channelCountMode. AccessType: inputOutput
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   * @param value The new value for channelCountMode.
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * @brief Gets the value of channelInterpretation. AccessType: inputOutput
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   * @return ChannelInterpretationChoices The current value of
   * channelInterpretation.
   */
  ChannelInterpretationChoices getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * @brief Sets the value of channelInterpretation. AccessType: inputOutput
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   * @param value The new value for channelInterpretation.
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

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
   * @brief Gets the value of length. AccessType: outputOnly
   * @details
   * @return SFInt32 The current value of length.
   */
  SFInt32 getLength() const { return _length; }

  /**
   * @brief Emit an output value on length. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitLength(const SFInt32 &value) { _length = value; }

  /**
   * @brief Gets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @return SFBool The current value of loop.
   */
  SFBool getLoop() const { return _loop; }

  /**
   * @brief Sets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @param value The new value for loop.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * @brief Gets the value of loopEnd. AccessType: inputOutput
   * @details loopEnd field is optional playhead position where looping ends if
   * loop=true.
   * @return SFFloat The current value of loopEnd.
   */
  SFFloat getLoopEnd() const { return _loopEnd; }

  /**
   * @brief Sets the value of loopEnd. AccessType: inputOutput
   * @details loopEnd field is optional playhead position where looping ends if
   * loop=true.
   * @param value The new value for loopEnd.
   */
  void setLoopEnd(const SFFloat &value) {

    validateLoopEnd(value);

    _loopEnd = value;
  }

  /**
   * @brief Non-validating write of loopEnd (runtime/reader ingest path).
   * @details Assigns without the range check setLoopEnd() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLoopEnd() stays the
   *          enforcement point for programmatic callers.
   */
  void setLoopEndUnchecked(const SFFloat &value) { _loopEnd = value; }

  /**
   * @brief Gets the value of loopStart. AccessType: inputOutput
   * @details loopStart field is optional playhead position where looping begins
   * if loop=true.
   * @return SFFloat The current value of loopStart.
   */
  SFFloat getLoopStart() const { return _loopStart; }

  /**
   * @brief Sets the value of loopStart. AccessType: inputOutput
   * @details loopStart field is optional playhead position where looping begins
   * if loop=true.
   * @param value The new value for loopStart.
   */
  void setLoopStart(const SFFloat &value) {

    validateLoopStart(value);

    _loopStart = value;
  }

  /**
   * @brief Non-validating write of loopStart (runtime/reader ingest path).
   * @details Assigns without the range check setLoopStart() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLoopStart() stays the
   *          enforcement point for programmatic callers.
   */
  void setLoopStartUnchecked(const SFFloat &value) { _loopStart = value; }

  /**
   * @brief Gets the value of numberOfChannels. AccessType: inputOutput
   * @details numberOfChannels is number of audio channels found in this buffer
   * source.
   * @return SFInt32 The current value of numberOfChannels.
   */
  SFInt32 getNumberOfChannels() const { return _numberOfChannels; }

  /**
   * @brief Sets the value of numberOfChannels. AccessType: inputOutput
   * @details numberOfChannels is number of audio channels found in this buffer
   * source.
   * @param value The new value for numberOfChannels.
   */
  void setNumberOfChannels(const SFInt32 &value) {

    validateNumberOfChannels(value);

    _numberOfChannels = value;
  }

  /**
   * @brief Non-validating write of numberOfChannels (runtime/reader ingest
   * path).
   * @details Assigns without the range check setNumberOfChannels() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setNumberOfChannels() stays the enforcement point for
   * programmatic callers.
   */
  void setNumberOfChannelsUnchecked(const SFInt32 &value) {
    _numberOfChannels = value;
  }

  /**
   * @brief Gets the value of playbackRate. AccessType: inputOutput
   * @details playbackRate field is speed at which to render the audio stream,
   * and forms a compound field together with detune field Hint: negative values
   * play in reverse.
   * @return SFFloat The current value of playbackRate.
   */
  SFFloat getPlaybackRate() const { return _playbackRate; }

  /**
   * @brief Sets the value of playbackRate. AccessType: inputOutput
   * @details playbackRate field is speed at which to render the audio stream,
   * and forms a compound field together with detune field Hint: negative values
   * play in reverse.
   * @param value The new value for playbackRate.
   */
  void setPlaybackRate(const SFFloat &value) { _playbackRate = value; }

  /**
   * @brief Gets the value of sampleRate. AccessType: inputOutput
   * @details sampleRate field is sample-frames per second.
   * @return SFFloat The current value of sampleRate.
   */
  SFFloat getSampleRate() const { return _sampleRate; }

  /**
   * @brief Sets the value of sampleRate. AccessType: inputOutput
   * @details sampleRate field is sample-frames per second.
   * @param value The new value for sampleRate.
   */
  void setSampleRate(const SFFloat &value) {

    validateSampleRate(value);

    _sampleRate = value;
  }

  /**
   * @brief Non-validating write of sampleRate (runtime/reader ingest path).
   * @details Assigns without the range check setSampleRate() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSampleRate() stays the
   *          enforcement point for programmatic callers.
   */
  void setSampleRateUnchecked(const SFFloat &value) { _sampleRate = value; }

  /**
   * @brief The X3D type name of this node (e.g. "BufferAudioSource").
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
  static void checkRangesBuffer(const MFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBufferDuration(const SFTime &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

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
  static void checkRangesLoopEnd(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesLoopStart(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesNumberOfChannels(const SFInt32 &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSampleRate(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateBuffer(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < -1.0f)
        throw std::out_of_range("buffer below minimum of -1");
      if (v > 1.0f)
        throw std::out_of_range("buffer above maximum of 1");
    }
  }

  static void validateBufferDuration(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("bufferDuration below minimum of 0");
  }

  static void validateDetune(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("detune below minimum of 0");
  }

  static void validateLoopEnd(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("loopEnd below minimum of 0");
  }

  static void validateLoopStart(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("loopStart below minimum of 0");
  }

  static void validateNumberOfChannels(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("numberOfChannels below minimum of 0");
  }

  static void validateSampleRate(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("sampleRate below minimum of 0");
  }

  /**
   * @brief Member variable for buffer.
   */

  MFFloat _buffer{};

  /**
   * @brief Member variable for bufferDuration.
   */

  SFTime _bufferDuration{0};

  /**
   * @brief Member variable for bufferlength.
   */

  SFInt32 _bufferlength{};

  /**
   * @brief Member variable for channelCount.
   */

  SFInt32 _channelCount{};

  /**
   * @brief Member variable for channelCountMode.
   */

  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  /**
   * @brief Member variable for channelInterpretation.
   */

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  /**
   * @brief Member variable for detune.
   */

  SFFloat _detune{0};

  /**
   * @brief Member variable for length.
   */

  SFInt32 _length{};

  /**
   * @brief Member variable for loop.
   */

  SFBool _loop{false};

  /**
   * @brief Member variable for loopEnd.
   */

  SFFloat _loopEnd{0};

  /**
   * @brief Member variable for loopStart.
   */

  SFFloat _loopStart{0};

  /**
   * @brief Member variable for numberOfChannels.
   */

  SFInt32 _numberOfChannels{0};

  /**
   * @brief Member variable for playbackRate.
   */

  SFFloat _playbackRate{1};

  /**
   * @brief Member variable for sampleRate.
   */

  SFFloat _sampleRate{0};
};

} // namespace x3d::nodes
