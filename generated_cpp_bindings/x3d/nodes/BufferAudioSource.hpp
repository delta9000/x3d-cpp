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
  BufferAudioSource() = default;

  ~BufferAudioSource() = default;

  static SFTime getDefaultBufferDuration() { return 0; }

  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  static SFFloat getDefaultDetune() { return 0; }

  static SFBool getDefaultLoop() { return false; }

  static SFFloat getDefaultLoopEnd() { return 0; }

  static SFFloat getDefaultLoopStart() { return 0; }

  static SFInt32 getDefaultNumberOfChannels() { return 0; }

  static SFFloat getDefaultPlaybackRate() { return 1; }

  static SFFloat getDefaultSampleRate() { return 0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   */
  const MFFloat &getBuffer() const { return _buffer; }

  /**
   * Access: inputOutput.
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBuffer() for range validation.
   */
  void setBufferUnchecked(const MFFloat &value) { _buffer = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details bufferDuration is duration in seconds to use from buffer field.
   */
  const SFTime &getBufferDuration() const { return _bufferDuration; }

  /**
   * Access: inputOutput.
   * @details bufferDuration is duration in seconds to use from buffer field.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBufferDuration() for range validation.
   */
  void setBufferDurationUnchecked(const SFTime &value) {
    _bufferDuration = value;
  }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getBufferlength() const { return _bufferlength; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitBufferlength(const SFInt32 &value) { _bufferlength = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details channelCount reports number of channels provided by input nodes.
   */
  const SFInt32 &getChannelCount() const { return _channelCount; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details channelCount reports number of channels provided by input nodes.
   */
  void emitChannelCount(const SFInt32 &value) { _channelCount = value; }

  /**
   * ChannelCountModeChoices; access: inputOutput. Borrows the stored value.
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   */
  const ChannelCountModeChoices &getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * Access: inputOutput.
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * ChannelInterpretationChoices; access: inputOutput. Borrows the stored
   * value.
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   */
  const ChannelInterpretationChoices &getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * Access: inputOutput.
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

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
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getLength() const { return _length; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitLength(const SFInt32 &value) { _length = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  const SFBool &getLoop() const { return _loop; }

  /**
   * Access: inputOutput.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details loopEnd field is optional playhead position where looping ends if
   * loop=true.
   */
  const SFFloat &getLoopEnd() const { return _loopEnd; }

  /**
   * Access: inputOutput.
   * @details loopEnd field is optional playhead position where looping ends if
   * loop=true.
   */
  void setLoopEnd(const SFFloat &value) {

    validateLoopEnd(value);

    _loopEnd = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLoopEnd() for range validation.
   */
  void setLoopEndUnchecked(const SFFloat &value) { _loopEnd = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details loopStart field is optional playhead position where looping begins
   * if loop=true.
   */
  const SFFloat &getLoopStart() const { return _loopStart; }

  /**
   * Access: inputOutput.
   * @details loopStart field is optional playhead position where looping begins
   * if loop=true.
   */
  void setLoopStart(const SFFloat &value) {

    validateLoopStart(value);

    _loopStart = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLoopStart() for range validation.
   */
  void setLoopStartUnchecked(const SFFloat &value) { _loopStart = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details numberOfChannels is number of audio channels found in this buffer
   * source.
   */
  const SFInt32 &getNumberOfChannels() const { return _numberOfChannels; }

  /**
   * Access: inputOutput.
   * @details numberOfChannels is number of audio channels found in this buffer
   * source.
   */
  void setNumberOfChannels(const SFInt32 &value) {

    validateNumberOfChannels(value);

    _numberOfChannels = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setNumberOfChannels() for range validation.
   */
  void setNumberOfChannelsUnchecked(const SFInt32 &value) {
    _numberOfChannels = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details playbackRate field is speed at which to render the audio stream,
   * and forms a compound field together with detune field Hint: negative values
   * play in reverse.
   */
  const SFFloat &getPlaybackRate() const { return _playbackRate; }

  /**
   * Access: inputOutput.
   * @details playbackRate field is speed at which to render the audio stream,
   * and forms a compound field together with detune field Hint: negative values
   * play in reverse.
   */
  void setPlaybackRate(const SFFloat &value) { _playbackRate = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details sampleRate field is sample-frames per second.
   */
  const SFFloat &getSampleRate() const { return _sampleRate; }

  /**
   * Access: inputOutput.
   * @details sampleRate field is sample-frames per second.
   */
  void setSampleRate(const SFFloat &value) {

    validateSampleRate(value);

    _sampleRate = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSampleRate() for range validation.
   */
  void setSampleRateUnchecked(const SFFloat &value) { _sampleRate = value; }

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

  MFFloat _buffer{};

  SFTime _bufferDuration{0};

  SFInt32 _bufferlength{};

  SFInt32 _channelCount{};

  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  SFFloat _detune{0};

  SFInt32 _length{};

  SFBool _loop{false};

  SFFloat _loopEnd{0};

  SFFloat _loopStart{0};

  SFInt32 _numberOfChannels{0};

  SFFloat _playbackRate{1};

  SFFloat _sampleRate{0};
};

} // namespace x3d::nodes
