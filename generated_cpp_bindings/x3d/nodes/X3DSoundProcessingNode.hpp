// X3DSoundProcessingNode.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSoundProcessingNode
 * @brief Base type for all sound processing nodes, which are used to enhance
 * audio with filtering, delaying, changing gain, etc.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#X3DSoundProcessingNode
 */
class X3DSoundProcessingNode : public virtual X3DTimeDependentNode,
                               public virtual X3DSoundNode {
public:
  /**
   * @brief Default constructor for X3DSoundProcessingNode
   */
  X3DSoundProcessingNode() = default;

  /**
   * @brief Virtual destructor for X3DSoundProcessingNode
   */
  virtual ~X3DSoundProcessingNode() = default;

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
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for gain
   * @return SFFloat The default value
   */
  static SFFloat getDefaultGain() { return 1; }

  /**
   * @brief Get the default value for tailTime
   * @return SFTime The default value
   */
  static SFTime getDefaultTailTime() { return 0; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
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
   * @brief Gets the value of channelCount. AccessType: outputOnly
   * @details
   * @return SFInt32 The current value of channelCount.
   */
  SFInt32 getChannelCount() const { return _channelCount; }

  /**
   * @brief Emit an output value on channelCount. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitChannelCount(const SFInt32 &value) { _channelCount = value; }

  /**
   * @brief Gets the value of channelCountMode. AccessType: inputOutput
   * @details
   * @return ChannelCountModeChoices The current value of channelCountMode.
   */
  ChannelCountModeChoices getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * @brief Sets the value of channelCountMode. AccessType: inputOutput
   * @details
   * @param value The new value for channelCountMode.
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * @brief Gets the value of channelInterpretation. AccessType: inputOutput
   * @details
   * @return ChannelInterpretationChoices The current value of
   * channelInterpretation.
   */
  ChannelInterpretationChoices getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * @brief Sets the value of channelInterpretation. AccessType: inputOutput
   * @details
   * @param value The new value for channelInterpretation.
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of gain. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of gain.
   */
  SFFloat getGain() const { return _gain; }

  /**
   * @brief Sets the value of gain. AccessType: inputOutput
   * @details
   * @param value The new value for gain.
   */
  void setGain(const SFFloat &value) { _gain = value; }

  /**
   * @brief Gets the value of tailTime. AccessType: inputOutput
   * @details
   * @return SFTime The current value of tailTime.
   */
  SFTime getTailTime() const { return _tailTime; }

  /**
   * @brief Sets the value of tailTime. AccessType: inputOutput
   * @details
   * @param value The new value for tailTime.
   */
  void setTailTime(const SFTime &value) {

    validateTailTime(value);

    _tailTime = value;
  }

  void setTailTime(SFTime &&value) {

    validateTailTime(value);

    _tailTime = std::move(value);
  }

  /**
   * @brief Non-validating write of tailTime (runtime/reader ingest path).
   * @details Assigns without the range check setTailTime() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTailTime() stays the
   *          enforcement point for programmatic callers.
   */
  void setTailTimeUnchecked(const SFTime &value) { _tailTime = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DSoundProcessingNode").
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
  static void checkRangesTailTime(const SFTime &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateTailTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("tailTime below minimum of 0");
  }

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
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for gain.
   */

  SFFloat _gain{1};

  /**
   * @brief Member variable for tailTime.
   */

  SFTime _tailTime{0};
};

} // namespace x3d::nodes
