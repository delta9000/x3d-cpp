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
  X3DSoundProcessingNode() = default;

  virtual ~X3DSoundProcessingNode() = default;

  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  static SFBool getDefaultEnabled() { return true; }

  static SFFloat getDefaultGain() { return 1; }

  static SFTime getDefaultTailTime() { return 0; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getChannelCount() const { return _channelCount; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitChannelCount(const SFInt32 &value) { _channelCount = value; }

  /**
   * ChannelCountModeChoices; access: inputOutput. Borrows the stored value.
   * @details
   */
  const ChannelCountModeChoices &getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * ChannelInterpretationChoices; access: inputOutput. Borrows the stored
   * value.
   * @details
   */
  const ChannelInterpretationChoices &getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getGain() const { return _gain; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGain(const SFFloat &value) { _gain = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getTailTime() const { return _tailTime; }

  /**
   * Access: inputOutput.
   * @details
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTailTime() for range validation.
   */
  void setTailTimeUnchecked(const SFTime &value) { _tailTime = value; }

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
  static void checkRangesTailTime(const SFTime &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateTailTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("tailTime below minimum of 0");
  }

  SFInt32 _channelCount{};

  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  SFBool _enabled{true};

  SFFloat _gain{1};

  SFTime _tailTime{0};
};

} // namespace x3d::nodes
