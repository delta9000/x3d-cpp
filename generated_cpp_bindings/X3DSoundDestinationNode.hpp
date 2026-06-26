// X3DSoundDestinationNode.hpp
#ifndef X3DSOUNDDESTINATIONNODE_HPP
#define X3DSOUNDDESTINATIONNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSoundNode.hpp"

/**
 * @class X3DSoundDestinationNode
 * @brief Base type for all sound destination nodes, which represent the final
 * destination of an audio signal and are what the user can ultimately hear.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#X3DSoundDestinationNode
 */
class X3DSoundDestinationNode : public virtual X3DSoundNode {
public:
  /**
   * @brief Default constructor for X3DSoundDestinationNode
   */
  X3DSoundDestinationNode() = default;

  /**
   * @brief Virtual destructor for X3DSoundDestinationNode
   */
  virtual ~X3DSoundDestinationNode() = default;

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
   * @brief Get the default value for gain
   * @return SFFloat The default value
   */
  static SFFloat getDefaultGain() { return 1; }

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
   * @brief Gets the value of isActive. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isActive.
   */
  SFBool getIsActive() const { return _isActive; }

  /**
   * @brief Emit an output value on isActive. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * @brief Gets the value of mediaDeviceID. AccessType: inputOutput
   * @details
   * @return SFString The current value of mediaDeviceID.
   */
  SFString getMediaDeviceID() const { return _mediaDeviceID; }

  /**
   * @brief Sets the value of mediaDeviceID. AccessType: inputOutput
   * @details
   * @param value The new value for mediaDeviceID.
   */
  void setMediaDeviceID(const SFString &value) { _mediaDeviceID = value; }

  void setMediaDeviceID(SFString &&value) { _mediaDeviceID = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DSoundDestinationNode").
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

private:
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
   * @brief Member variable for gain.
   */

  SFFloat _gain{1};

  /**
   * @brief Member variable for isActive.
   */

  SFBool _isActive{};

  /**
   * @brief Member variable for mediaDeviceID.
   */

  SFString _mediaDeviceID{};
};

#endif // X3DSOUNDDESTINATIONNODE_HPP