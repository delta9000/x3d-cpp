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

#include "x3d/nodes/X3DSoundNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSoundDestinationNode
 * @brief Base type for all sound destination nodes, which represent the final
 * destination of an audio signal and are what the user can ultimately hear.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#X3DSoundDestinationNode
 */
class X3DSoundDestinationNode : public virtual X3DSoundNode {
public:
  X3DSoundDestinationNode() = default;

  virtual ~X3DSoundDestinationNode() = default;

  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  static SFFloat getDefaultGain() { return 1; }

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
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsActive() const { return _isActive; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getMediaDeviceID() const { return _mediaDeviceID; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setMediaDeviceID(const SFString &value) { _mediaDeviceID = value; }

  void setMediaDeviceID(SFString &&value) { _mediaDeviceID = std::move(value); }

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

private:
  SFInt32 _channelCount{};

  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  SFFloat _gain{1};

  SFBool _isActive{};

  SFString _mediaDeviceID{};
};

} // namespace x3d::nodes
