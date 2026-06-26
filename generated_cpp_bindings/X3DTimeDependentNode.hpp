// X3DTimeDependentNode.hpp
#ifndef X3DTIMEDEPENDENTNODE_HPP
#define X3DTIMEDEPENDENTNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

/**
 * @class X3DTimeDependentNode
 * @brief Base type from which all time-dependent nodes are derived.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html#X3DTimeDependentNode
 */
class X3DTimeDependentNode : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for X3DTimeDependentNode
   */
  X3DTimeDependentNode() = default;

  /**
   * @brief Virtual destructor for X3DTimeDependentNode
   */
  virtual ~X3DTimeDependentNode() = default;

  /**
   * @brief Get the default value for pauseTime
   * @return SFTime The default value
   */
  static SFTime getDefaultPauseTime() { return 0; }

  /**
   * @brief Get the default value for resumeTime
   * @return SFTime The default value
   */
  static SFTime getDefaultResumeTime() { return 0; }

  /**
   * @brief Get the default value for startTime
   * @return SFTime The default value
   */
  static SFTime getDefaultStartTime() { return 0; }

  /**
   * @brief Get the default value for stopTime
   * @return SFTime The default value
   */
  static SFTime getDefaultStopTime() { return 0; }

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
  static std::string componentName() { return "Time"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of elapsedTime. AccessType: outputOnly
   * @details
   * @return SFTime The current value of elapsedTime.
   */
  SFTime getElapsedTime() const { return _elapsedTime; }

  /**
   * @brief Emit an output value on elapsedTime. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitElapsedTime(const SFTime &value) { _elapsedTime = value; }

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
   * @brief Gets the value of isPaused. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isPaused.
   */
  SFBool getIsPaused() const { return _isPaused; }

  /**
   * @brief Emit an output value on isPaused. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsPaused(const SFBool &value) { _isPaused = value; }

  /**
   * @brief Gets the value of pauseTime. AccessType: inputOutput
   * @details
   * @return SFTime The current value of pauseTime.
   */
  SFTime getPauseTime() const { return _pauseTime; }

  /**
   * @brief Sets the value of pauseTime. AccessType: inputOutput
   * @details
   * @param value The new value for pauseTime.
   */
  void setPauseTime(const SFTime &value) { _pauseTime = value; }

  void setPauseTime(SFTime &&value) { _pauseTime = std::move(value); }

  /**
   * @brief Gets the value of resumeTime. AccessType: inputOutput
   * @details
   * @return SFTime The current value of resumeTime.
   */
  SFTime getResumeTime() const { return _resumeTime; }

  /**
   * @brief Sets the value of resumeTime. AccessType: inputOutput
   * @details
   * @param value The new value for resumeTime.
   */
  void setResumeTime(const SFTime &value) { _resumeTime = value; }

  void setResumeTime(SFTime &&value) { _resumeTime = std::move(value); }

  /**
   * @brief Gets the value of startTime. AccessType: inputOutput
   * @details
   * @return SFTime The current value of startTime.
   */
  SFTime getStartTime() const { return _startTime; }

  /**
   * @brief Sets the value of startTime. AccessType: inputOutput
   * @details
   * @param value The new value for startTime.
   */
  void setStartTime(const SFTime &value) { _startTime = value; }

  void setStartTime(SFTime &&value) { _startTime = std::move(value); }

  /**
   * @brief Gets the value of stopTime. AccessType: inputOutput
   * @details
   * @return SFTime The current value of stopTime.
   */
  SFTime getStopTime() const { return _stopTime; }

  /**
   * @brief Sets the value of stopTime. AccessType: inputOutput
   * @details
   * @param value The new value for stopTime.
   */
  void setStopTime(const SFTime &value) { _stopTime = value; }

  void setStopTime(SFTime &&value) { _stopTime = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DTimeDependentNode").
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
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for elapsedTime.
   */

  SFTime _elapsedTime{};

  /**
   * @brief Member variable for isActive.
   */

  SFBool _isActive{};

  /**
   * @brief Member variable for isPaused.
   */

  SFBool _isPaused{};

  /**
   * @brief Member variable for pauseTime.
   */

  SFTime _pauseTime{0};

  /**
   * @brief Member variable for resumeTime.
   */

  SFTime _resumeTime{0};

  /**
   * @brief Member variable for startTime.
   */

  SFTime _startTime{0};

  /**
   * @brief Member variable for stopTime.
   */

  SFTime _stopTime{0};
};

#endif // X3DTIMEDEPENDENTNODE_HPP