// HAnimMotion.hpp
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
  /**
   * @brief Default constructor for HAnimMotion
   */
  HAnimMotion() = default;

  /**
   * @brief Destructor for HAnimMotion
   */
  ~HAnimMotion() = default;

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for endFrame
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEndFrame() { return 0; }

  /**
   * @brief Get the default value for frameDuration
   * @return SFTime The default value
   */
  static SFTime getDefaultFrameDuration() { return 0.1; }

  /**
   * @brief Get the default value for frameIncrement
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFrameIncrement() { return 1; }

  /**
   * @brief Get the default value for frameIndex
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFrameIndex() { return 0; }

  /**
   * @brief Get the default value for loa
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultLoa() { return -1; }

  /**
   * @brief Get the default value for loop
   * @return SFBool The default value
   */
  static SFBool getDefaultLoop() { return false; }

  /**
   * @brief Get the default value for startFrame
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultStartFrame() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimMotion";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "motions"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "HAnim"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of channels. AccessType: inputOutput
   * @details list of number of channels for transformation, followed by
   * transformation type of each channel of data.
   * @return SFString The current value of channels.
   */
  SFString getChannels() const { return _channels; }

  /**
   * @brief Sets the value of channels. AccessType: inputOutput
   * @details list of number of channels for transformation, followed by
   * transformation type of each channel of data.
   * @param value The new value for channels.
   */
  void setChannels(const SFString &value) { _channels = value; }

  void setChannels(SFString &&value) { _channels = std::move(value); }

  /**
   * @brief Gets the value of channelsEnabled. AccessType: inputOutput
   * @details boolean values for each channel indicating whether enabled.
   * @return MFBool The current value of channelsEnabled.
   */
  MFBool getChannelsEnabled() const { return _channelsEnabled; }

  /**
   * @brief Sets the value of channelsEnabled. AccessType: inputOutput
   * @details boolean values for each channel indicating whether enabled.
   * @param value The new value for channelsEnabled.
   */
  void setChannelsEnabled(const MFBool &value) { _channelsEnabled = value; }

  void setChannelsEnabled(MFBool &&value) {

    _channelsEnabled = std::move(value);
  }

  /**
   * @brief Gets the value of cycleTime. AccessType: outputOnly
   * @details cycleTime sends a time event at initial starting time and at
   * beginning of each new cycle.
   * @return SFTime The current value of cycleTime.
   */
  SFTime getCycleTime() const { return _cycleTime; }

  /**
   * @brief Emit an output value on cycleTime. AccessType: outputOnly
   * @details cycleTime sends a time event at initial starting time and at
   * beginning of each new cycle. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitCycleTime(const SFTime &value) { _cycleTime = value; }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of elapsedTime. AccessType: outputOnly
   * @details elapsedTime is computed elapsed time since the Motion object was
   * activated and running, counting all traversed frames (as if frameIndex
   * equaled 1) and multiplied by frameDuration, cumulative in seconds.
   * @return SFTime The current value of elapsedTime.
   */
  SFTime getElapsedTime() const { return _elapsedTime; }

  /**
   * @brief Emit an output value on elapsedTime. AccessType: outputOnly
   * @details elapsedTime is computed elapsed time since the Motion object was
   * activated and running, counting all traversed frames (as if frameIndex
   * equaled 1) and multiplied by frameDuration, cumulative in seconds.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitElapsedTime(const SFTime &value) { _elapsedTime = value; }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of endFrame. AccessType: inputOutput
   * @details endFrame indicates final index of animated frame.
   * @return SFInt32 The current value of endFrame.
   */
  SFInt32 getEndFrame() const { return _endFrame; }

  /**
   * @brief Sets the value of endFrame. AccessType: inputOutput
   * @details endFrame indicates final index of animated frame.
   * @param value The new value for endFrame.
   */
  void setEndFrame(const SFInt32 &value) {

    validateEndFrame(value);

    _endFrame = value;
  }

  /**
   * @brief Non-validating write of endFrame (runtime/reader ingest path).
   * @details Assigns without the range check setEndFrame() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setEndFrame() stays the
   *          enforcement point for programmatic callers.
   */
  void setEndFrameUnchecked(const SFInt32 &value) { _endFrame = value; }

  /**
   * @brief Gets the value of frameCount. AccessType: outputOnly
   * @details frameCount is computed at run time and indicates the total number
   * of frames present in the animation, equaling the number of sets of channel
   * data rows present in the values array.
   * @return SFInt32 The current value of frameCount.
   */
  SFInt32 getFrameCount() const { return _frameCount; }

  /**
   * @brief Emit an output value on frameCount. AccessType: outputOnly
   * @details frameCount is computed at run time and indicates the total number
   * of frames present in the animation, equaling the number of sets of channel
   * data rows present in the values array. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitFrameCount(const SFInt32 &value) { _frameCount = value; }

  /**
   * @brief Gets the value of frameDuration. AccessType: inputOutput
   * @details frameDuration specifies the duration of each frame in seconds.
   * @return SFTime The current value of frameDuration.
   */
  SFTime getFrameDuration() const { return _frameDuration; }

  /**
   * @brief Sets the value of frameDuration. AccessType: inputOutput
   * @details frameDuration specifies the duration of each frame in seconds.
   * @param value The new value for frameDuration.
   */
  void setFrameDuration(const SFTime &value) { _frameDuration = value; }

  void setFrameDuration(SFTime &&value) { _frameDuration = std::move(value); }

  /**
   * @brief Gets the value of frameIncrement. AccessType: inputOutput
   * @details frameIncrement field controls whether playback direction is
   * forwards or backwards, and also whether frames are skipped (for example,
   * subsampled replay).
   * @return SFInt32 The current value of frameIncrement.
   */
  SFInt32 getFrameIncrement() const { return _frameIncrement; }

  /**
   * @brief Sets the value of frameIncrement. AccessType: inputOutput
   * @details frameIncrement field controls whether playback direction is
   * forwards or backwards, and also whether frames are skipped (for example,
   * subsampled replay).
   * @param value The new value for frameIncrement.
   */
  void setFrameIncrement(const SFInt32 &value) { _frameIncrement = value; }

  /**
   * @brief Gets the value of frameIndex. AccessType: inputOutput
   * @details frameIndex indicates index of current frame.
   * @return SFInt32 The current value of frameIndex.
   */
  SFInt32 getFrameIndex() const { return _frameIndex; }

  /**
   * @brief Sets the value of frameIndex. AccessType: inputOutput
   * @details frameIndex indicates index of current frame.
   * @param value The new value for frameIndex.
   */
  void setFrameIndex(const SFInt32 &value) {

    validateFrameIndex(value);

    _frameIndex = value;
  }

  /**
   * @brief Non-validating write of frameIndex (runtime/reader ingest path).
   * @details Assigns without the range check setFrameIndex() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFrameIndex() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrameIndexUnchecked(const SFInt32 &value) { _frameIndex = value; }

  /**
   * @brief Gets the value of joints. AccessType: inputOutput
   * @details joints field lists names of joints that raw motion data is to be
   * applied to.
   * @return SFString The current value of joints.
   */
  SFString getJoints() const { return _joints; }

  /**
   * @brief Sets the value of joints. AccessType: inputOutput
   * @details joints field lists names of joints that raw motion data is to be
   * applied to.
   * @param value The new value for joints.
   */
  void setJoints(const SFString &value) { _joints = value; }

  void setJoints(SFString &&value) { _joints = std::move(value); }

  /**
   * @brief Gets the value of loa. AccessType: inputOutput
   * @details Level Of Articulation 0.
   * @return SFInt32 The current value of loa.
   */
  SFInt32 getLoa() const { return _loa; }

  /**
   * @brief Sets the value of loa. AccessType: inputOutput
   * @details Level Of Articulation 0.
   * @param value The new value for loa.
   */
  void setLoa(const SFInt32 &value) {

    validateLoa(value);

    _loa = value;
  }

  /**
   * @brief Non-validating write of loa (runtime/reader ingest path).
   * @details Assigns without the range check setLoa() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLoa() stays the
   *          enforcement point for programmatic callers.
   */
  void setLoaUnchecked(const SFInt32 &value) { _loa = value; }

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
   * @brief Gets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimMotion node can
   * be identified at run time for animation purposes.
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimMotion node can
   * be identified at run time for animation purposes.
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief Event handler invoked when an event is received on next. AccessType:
   * inputOnly
   * @details Send next output value in values array, using/updating various
   * frame values as appropriate. Dispatches to the handler registered via
   * setOnNextHandler(); a no-op if none is set. The event cascade reaches this
   * through the node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onNext(const SFBool &value) {
    if (_onNextHandler)
      _onNextHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on next.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnNextHandler(std::function<void(const SFBool &)> handler) {
    _onNextHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on previous.
   * AccessType: inputOnly
   * @details Send previous output value in values array, using/updating various
   * frame values as appropriate. Dispatches to the handler registered via
   * setOnPreviousHandler(); a no-op if none is set. The event cascade reaches
   * this through the node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onPrevious(const SFBool &value) {
    if (_onPreviousHandler)
      _onPreviousHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on previous.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnPreviousHandler(std::function<void(const SFBool &)> handler) {
    _onPreviousHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of startFrame. AccessType: inputOutput
   * @details startFrame indicates initial index of animated frame.
   * @return SFInt32 The current value of startFrame.
   */
  SFInt32 getStartFrame() const { return _startFrame; }

  /**
   * @brief Sets the value of startFrame. AccessType: inputOutput
   * @details startFrame indicates initial index of animated frame.
   * @param value The new value for startFrame.
   */
  void setStartFrame(const SFInt32 &value) {

    validateStartFrame(value);

    _startFrame = value;
  }

  /**
   * @brief Non-validating write of startFrame (runtime/reader ingest path).
   * @details Assigns without the range check setStartFrame() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setStartFrame() stays the
   *          enforcement point for programmatic callers.
   */
  void setStartFrameUnchecked(const SFInt32 &value) { _startFrame = value; }

  /**
   * @brief Gets the value of values. AccessType: inputOutput
   * @details values field contains all transformation values, ordered first by
   * frame, then by joint, and then by transformation Sets of floats in the
   * values array matching the order listed in joints and channels fields.
   * @return MFFloat The current value of values.
   */
  MFFloat getValues() const { return _values; }

  /**
   * @brief Sets the value of values. AccessType: inputOutput
   * @details values field contains all transformation values, ordered first by
   * frame, then by joint, and then by transformation Sets of floats in the
   * values array matching the order listed in joints and channels fields.
   * @param value The new value for values.
   */
  void setValues(const MFFloat &value) { _values = value; }

  void setValues(MFFloat &&value) { _values = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "HAnimMotion").
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

  /**
   * @brief Member variable for channels.
   */

  SFString _channels{};

  /**
   * @brief Member variable for channelsEnabled.
   */

  MFBool _channelsEnabled{};

  /**
   * @brief Member variable for cycleTime.
   */

  SFTime _cycleTime{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for elapsedTime.
   */

  SFTime _elapsedTime{};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for endFrame.
   */

  SFInt32 _endFrame{0};

  /**
   * @brief Member variable for frameCount.
   */

  SFInt32 _frameCount{};

  /**
   * @brief Member variable for frameDuration.
   */

  SFTime _frameDuration{0.1};

  /**
   * @brief Member variable for frameIncrement.
   */

  SFInt32 _frameIncrement{1};

  /**
   * @brief Member variable for frameIndex.
   */

  SFInt32 _frameIndex{0};

  /**
   * @brief Member variable for joints.
   */

  SFString _joints{};

  /**
   * @brief Member variable for loa.
   */

  SFInt32 _loa{-1};

  /**
   * @brief Member variable for loop.
   */

  SFBool _loop{false};

  /**
   * @brief Member variable for name.
   */

  SFString _name{};

  /**
   * @brief Registered event handler for next (inputOnly); empty until set.
   */
  std::function<void(const SFBool &)> _onNextHandler{};

  /**
   * @brief Registered event handler for previous (inputOnly); empty until set.
   */
  std::function<void(const SFBool &)> _onPreviousHandler{};

  /**
   * @brief Member variable for startFrame.
   */

  SFInt32 _startFrame{0};

  /**
   * @brief Member variable for values.
   */

  MFFloat _values{};
};

} // namespace x3d::nodes
