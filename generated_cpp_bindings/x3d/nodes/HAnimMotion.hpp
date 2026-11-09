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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimMotion
 * @brief An HAnimMotion node supports discrete frame-by-frame playback for
 * HAnim motion data animation.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimMotion
 */
class HAnimMotion : public virtual X3DChildNode {
public:
  HAnimMotion() = default;

  ~HAnimMotion() = default;

  static SFBool getDefaultEnabled() { return true; }

  static SFInt32 getDefaultEndFrame() { return 0; }

  static SFTime getDefaultFrameDuration() { return 0.1; }

  static SFInt32 getDefaultFrameIncrement() { return 1; }

  static SFInt32 getDefaultFrameIndex() { return 0; }

  static SFInt32 getDefaultLoa() { return -1; }

  static SFBool getDefaultLoop() { return false; }

  static SFInt32 getDefaultStartFrame() { return 0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimMotion";
  }

  static std::string getDefaultContainerField() { return "motions"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 2; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details list of number of channels for transformation, followed by
   * transformation type of each channel of data.
   */
  const SFString &getChannels() const { return _channels; }

  /**
   * Access: inputOutput.
   * @details list of number of channels for transformation, followed by
   * transformation type of each channel of data.
   */
  void setChannels(const SFString &value) { _channels = value; }

  void setChannels(SFString &&value) { _channels = std::move(value); }

  /**
   * MFBool; access: inputOutput. Borrows the stored value.
   * @details boolean values for each channel indicating whether enabled.
   */
  const MFBool &getChannelsEnabled() const { return _channelsEnabled; }

  /**
   * Access: inputOutput.
   * @details boolean values for each channel indicating whether enabled.
   */
  void setChannelsEnabled(const MFBool &value) { _channelsEnabled = value; }

  void setChannelsEnabled(MFBool &&value) {

    _channelsEnabled = std::move(value);
  }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details cycleTime sends a time event at initial starting time and at
   * beginning of each new cycle.
   */
  const SFTime &getCycleTime() const { return _cycleTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details cycleTime sends a time event at initial starting time and at
   * beginning of each new cycle.
   */
  void emitCycleTime(const SFTime &value) { _cycleTime = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Author-provided prose that describes intended purpose of this
   * node.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Author-provided prose that describes intended purpose of this
   * node.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details elapsedTime is computed elapsed time since the Motion object was
   * activated and running, counting all traversed frames (as if frameIndex
   * equaled 1) and multiplied by frameDuration, cumulative in seconds.
   */
  const SFTime &getElapsedTime() const { return _elapsedTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details elapsedTime is computed elapsed time since the Motion object was
   * activated and running, counting all traversed frames (as if frameIndex
   * equaled 1) and multiplied by frameDuration, cumulative in seconds.
   */
  void emitElapsedTime(const SFTime &value) { _elapsedTime = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables node operation.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables node operation.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details endFrame indicates final index of animated frame.
   */
  const SFInt32 &getEndFrame() const { return _endFrame; }

  /**
   * Access: inputOutput.
   * @details endFrame indicates final index of animated frame.
   */
  void setEndFrame(const SFInt32 &value) {

    validateEndFrame(value);

    _endFrame = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setEndFrame() for range validation.
   */
  void setEndFrameUnchecked(const SFInt32 &value) { _endFrame = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details frameCount is computed at run time and indicates the total number
   * of frames present in the animation, equaling the number of sets of channel
   * data rows present in the values array.
   */
  const SFInt32 &getFrameCount() const { return _frameCount; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details frameCount is computed at run time and indicates the total number
   * of frames present in the animation, equaling the number of sets of channel
   * data rows present in the values array.
   */
  void emitFrameCount(const SFInt32 &value) { _frameCount = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details frameDuration specifies the duration of each frame in seconds.
   */
  const SFTime &getFrameDuration() const { return _frameDuration; }

  /**
   * Access: inputOutput.
   * @details frameDuration specifies the duration of each frame in seconds.
   */
  void setFrameDuration(const SFTime &value) { _frameDuration = value; }

  void setFrameDuration(SFTime &&value) { _frameDuration = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details frameIncrement field controls whether playback direction is
   * forwards or backwards, and also whether frames are skipped (for example,
   * subsampled replay).
   */
  const SFInt32 &getFrameIncrement() const { return _frameIncrement; }

  /**
   * Access: inputOutput.
   * @details frameIncrement field controls whether playback direction is
   * forwards or backwards, and also whether frames are skipped (for example,
   * subsampled replay).
   */
  void setFrameIncrement(const SFInt32 &value) { _frameIncrement = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details frameIndex indicates index of current frame.
   */
  const SFInt32 &getFrameIndex() const { return _frameIndex; }

  /**
   * Access: inputOutput.
   * @details frameIndex indicates index of current frame.
   */
  void setFrameIndex(const SFInt32 &value) {

    validateFrameIndex(value);

    _frameIndex = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrameIndex() for range validation.
   */
  void setFrameIndexUnchecked(const SFInt32 &value) { _frameIndex = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details joints field lists names of joints that raw motion data is to be
   * applied to.
   */
  const SFString &getJoints() const { return _joints; }

  /**
   * Access: inputOutput.
   * @details joints field lists names of joints that raw motion data is to be
   * applied to.
   */
  void setJoints(const SFString &value) { _joints = value; }

  void setJoints(SFString &&value) { _joints = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Level Of Articulation 0.
   */
  const SFInt32 &getLoa() const { return _loa; }

  /**
   * Access: inputOutput.
   * @details Level Of Articulation 0.
   */
  void setLoa(const SFInt32 &value) {

    validateLoa(value);

    _loa = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLoa() for range validation.
   */
  void setLoaUnchecked(const SFInt32 &value) { _loa = value; }

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
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that HAnimMotion node can
   * be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that HAnimMotion node can
   * be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * SFBool; access: inputOnly.
   * @details Send next output value in values array, using/updating various
   * frame values as appropriate. Dispatches to the handler registered via
   * setOnNextHandler(); a no-op if none is set.
   */
  void onNext(const SFBool &value) {
    if (_onNextHandler)
      _onNextHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnNextHandler(std::function<void(const SFBool &)> handler) {
    _onNextHandler = std::move(handler);
  }

  /**
   * SFBool; access: inputOnly.
   * @details Send previous output value in values array, using/updating various
   * frame values as appropriate. Dispatches to the handler registered via
   * setOnPreviousHandler(); a no-op if none is set.
   */
  void onPrevious(const SFBool &value) {
    if (_onPreviousHandler)
      _onPreviousHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnPreviousHandler(std::function<void(const SFBool &)> handler) {
    _onPreviousHandler = std::move(handler);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details startFrame indicates initial index of animated frame.
   */
  const SFInt32 &getStartFrame() const { return _startFrame; }

  /**
   * Access: inputOutput.
   * @details startFrame indicates initial index of animated frame.
   */
  void setStartFrame(const SFInt32 &value) {

    validateStartFrame(value);

    _startFrame = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStartFrame() for range validation.
   */
  void setStartFrameUnchecked(const SFInt32 &value) { _startFrame = value; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details values field contains all transformation values, ordered first by
   * frame, then by joint, and then by transformation Sets of floats in the
   * values array matching the order listed in joints and channels fields.
   */
  const MFFloat &getValues() const { return _values; }

  /**
   * Access: inputOutput.
   * @details values field contains all transformation values, ordered first by
   * frame, then by joint, and then by transformation Sets of floats in the
   * values array matching the order listed in joints and channels fields.
   */
  void setValues(const MFFloat &value) { _values = value; }

  void setValues(MFFloat &&value) { _values = std::move(value); }

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
  static void checkRangesEndFrame(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrameIndex(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesLoa(const SFInt32 &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStartFrame(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateEndFrame(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("endFrame below minimum of 0");
  }

  static void validateFrameIndex(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("frameIndex below minimum of 0");
  }

  static void validateLoa(const SFInt32 &value) {

    if (value < -1)
      throw std::out_of_range("loa below minimum of -1");
    if (value > 4)
      throw std::out_of_range("loa above maximum of 4");
  }

  static void validateStartFrame(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("startFrame below minimum of 0");
  }

  SFString _channels{};

  MFBool _channelsEnabled{};

  SFTime _cycleTime{};

  SFString _description{};

  SFTime _elapsedTime{};

  SFBool _enabled{true};

  SFInt32 _endFrame{0};

  SFInt32 _frameCount{};

  SFTime _frameDuration{0.1};

  SFInt32 _frameIncrement{1};

  SFInt32 _frameIndex{0};

  SFString _joints{};

  SFInt32 _loa{-1};

  SFBool _loop{false};

  SFString _name{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onNextHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onPreviousHandler{};

  SFInt32 _startFrame{0};

  MFFloat _values{};
};

} // namespace x3d::nodes
