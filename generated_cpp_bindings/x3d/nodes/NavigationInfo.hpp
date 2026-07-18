// NavigationInfo.hpp
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

#include "x3d/nodes/X3DBindableNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NavigationInfo
 * @brief NavigationInfo describes the user's viewing model, user
 * navigation-interaction modalities, and also dimensional characteristics of
 * the user's (typically invisible) avatar.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#NavigationInfo
 */
class NavigationInfo : public virtual X3DBindableNode {
public:
  /**
   * @brief Default constructor for NavigationInfo
   */
  NavigationInfo() = default;

  /**
   * @brief Destructor for NavigationInfo
   */
  ~NavigationInfo() = default;

  /**
   * @brief Get the default value for avatarSize
   * @return MFFloat The default value
   */
  static MFFloat getDefaultAvatarSize() {
    return std::vector<float>{0.25, 1.6, 0.75};
  }

  /**
   * @brief Get the default value for headlight
   * @return SFBool The default value
   */
  static SFBool getDefaultHeadlight() { return true; }

  /**
   * @brief Get the default value for speed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpeed() { return 1; }

  /**
   * @brief Get the default value for transitionTime
   * @return SFTime The default value
   */
  static SFTime getDefaultTransitionTime() { return 1.0; }

  /**
   * @brief Get the default value for transitionType
   * @return MFString The default value
   */
  static MFString getDefaultTransitionType() {
    return std::vector<std::string>{"LINEAR"};
  }

  /**
   * @brief Get the default value for type
   * @return MFString The default value
   */
  static MFString getDefaultType() {
    return std::vector<std::string>{"EXAMINE", "ANY"};
  }

  /**
   * @brief Get the default value for visibilityLimit
   * @return SFFloat The default value
   */
  static SFFloat getDefaultVisibilityLimit() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of avatarSize. AccessType: inputOutput
   * @details avatarSize triplet values define three separate parameters: (a)
   * collisionDistance between user and geometry, i.
   * @return MFFloat The current value of avatarSize.
   */
  MFFloat getAvatarSize() const { return _avatarSize; }

  /**
   * @brief Sets the value of avatarSize. AccessType: inputOutput
   * @details avatarSize triplet values define three separate parameters: (a)
   * collisionDistance between user and geometry, i.
   * @param value The new value for avatarSize.
   */
  void setAvatarSize(const MFFloat &value) {

    validateAvatarSize(value);

    _avatarSize = value;
  }

  void setAvatarSize(MFFloat &&value) {

    validateAvatarSize(value);

    _avatarSize = std::move(value);
  }

  /**
   * @brief Non-validating write of avatarSize (runtime/reader ingest path).
   * @details Assigns without the range check setAvatarSize() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAvatarSize() stays the
   *          enforcement point for programmatic callers.
   */
  void setAvatarSizeUnchecked(const MFFloat &value) { _avatarSize = value; }

  /**
   * @brief Gets the value of headlight. AccessType: inputOutput
   * @details Enable/disable directional light that always points in the
   * direction the user is looking.
   * @return SFBool The current value of headlight.
   */
  SFBool getHeadlight() const { return _headlight; }

  /**
   * @brief Sets the value of headlight. AccessType: inputOutput
   * @details Enable/disable directional light that always points in the
   * direction the user is looking.
   * @param value The new value for headlight.
   */
  void setHeadlight(const SFBool &value) { _headlight = value; }

  /**
   * @brief Gets the value of speed. AccessType: inputOutput
   * @details Default rate at which viewer travels through scene, meters/second.
   * @return SFFloat The current value of speed.
   */
  SFFloat getSpeed() const { return _speed; }

  /**
   * @brief Sets the value of speed. AccessType: inputOutput
   * @details Default rate at which viewer travels through scene, meters/second.
   * @param value The new value for speed.
   */
  void setSpeed(const SFFloat &value) {

    validateSpeed(value);

    _speed = value;
  }

  /**
   * @brief Non-validating write of speed (runtime/reader ingest path).
   * @details Assigns without the range check setSpeed() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSpeed() stays the
   *          enforcement point for programmatic callers.
   */
  void setSpeedUnchecked(const SFFloat &value) { _speed = value; }

  /**
   * @brief Gets the value of transitionComplete. AccessType: outputOnly
   * @details Event signaling viewpoint transition complete.
   * @return SFBool The current value of transitionComplete.
   */
  SFBool getTransitionComplete() const { return _transitionComplete; }

  /**
   * @brief Emit an output value on transitionComplete. AccessType: outputOnly
   * @details Event signaling viewpoint transition complete.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTransitionComplete(const SFBool &value) {
    _transitionComplete = value;
  }

  /**
   * @brief Gets the value of transitionTime. AccessType: inputOutput
   * @details transitionTime defines the expected duration of viewpoint
   * transition in seconds.
   * @return SFTime The current value of transitionTime.
   */
  SFTime getTransitionTime() const { return _transitionTime; }

  /**
   * @brief Sets the value of transitionTime. AccessType: inputOutput
   * @details transitionTime defines the expected duration of viewpoint
   * transition in seconds.
   * @param value The new value for transitionTime.
   */
  void setTransitionTime(const SFTime &value) {

    validateTransitionTime(value);

    _transitionTime = value;
  }

  void setTransitionTime(SFTime &&value) {

    validateTransitionTime(value);

    _transitionTime = std::move(value);
  }

  /**
   * @brief Non-validating write of transitionTime (runtime/reader ingest path).
   * @details Assigns without the range check setTransitionTime() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTransitionTime() stays the
   *          enforcement point for programmatic callers.
   */
  void setTransitionTimeUnchecked(const SFTime &value) {
    _transitionTime = value;
  }

  /**
   * @brief Gets the value of transitionType. AccessType: inputOutput
   * @details Camera transition between viewpoints.
   * @return MFString The current value of transitionType.
   */
  MFString getTransitionType() const { return _transitionType; }

  /**
   * @brief Sets the value of transitionType. AccessType: inputOutput
   * @details Camera transition between viewpoints.
   * @param value The new value for transitionType.
   */
  void setTransitionType(const MFString &value) { _transitionType = value; }

  void setTransitionType(MFString &&value) {

    _transitionType = std::move(value);
  }

  /**
   * @brief Gets the value of type. AccessType: inputOutput
   * @details Enter one or more quoted SFString values: "EXAMINE" "WALK" "FLY"
   * "LOOKAT" "EXPLORE" "ANY" "NONE".
   * @return MFString The current value of type.
   */
  MFString getType() const { return _type; }

  /**
   * @brief Sets the value of type. AccessType: inputOutput
   * @details Enter one or more quoted SFString values: "EXAMINE" "WALK" "FLY"
   * "LOOKAT" "EXPLORE" "ANY" "NONE".
   * @param value The new value for type.
   */
  void setType(const MFString &value) { _type = value; }

  void setType(MFString &&value) { _type = std::move(value); }

  /**
   * @brief Gets the value of visibilityLimit. AccessType: inputOutput
   * @details Geometry beyond the visibilityLimit may not be rendered (far
   * clipping plane of the view frustrum).
   * @return SFFloat The current value of visibilityLimit.
   */
  SFFloat getVisibilityLimit() const { return _visibilityLimit; }

  /**
   * @brief Sets the value of visibilityLimit. AccessType: inputOutput
   * @details Geometry beyond the visibilityLimit may not be rendered (far
   * clipping plane of the view frustrum).
   * @param value The new value for visibilityLimit.
   */
  void setVisibilityLimit(const SFFloat &value) {

    validateVisibilityLimit(value);

    _visibilityLimit = value;
  }

  /**
   * @brief Non-validating write of visibilityLimit (runtime/reader ingest
   * path).
   * @details Assigns without the range check setVisibilityLimit() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setVisibilityLimit() stays the enforcement point for
   * programmatic callers.
   */
  void setVisibilityLimitUnchecked(const SFFloat &value) {
    _visibilityLimit = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "NavigationInfo").
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
  static void checkRangesAvatarSize(const MFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSpeed(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTransitionTime(const SFTime &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVisibilityLimit(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateAvatarSize(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("avatarSize below minimum of 0");
    }
  }

  static void validateSpeed(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("speed below minimum of 0");
  }

  static void validateTransitionTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("transitionTime below minimum of 0");
  }

  static void validateVisibilityLimit(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("visibilityLimit below minimum of 0");
  }

  /**
   * @brief Member variable for avatarSize.
   */

  MFFloat _avatarSize{std::vector<float>{0.25, 1.6, 0.75}};

  /**
   * @brief Member variable for headlight.
   */

  SFBool _headlight{true};

  /**
   * @brief Member variable for speed.
   */

  SFFloat _speed{1};

  /**
   * @brief Member variable for transitionComplete.
   */

  SFBool _transitionComplete{};

  /**
   * @brief Member variable for transitionTime.
   */

  SFTime _transitionTime{1.0};

  /**
   * @brief Member variable for transitionType.
   */

  MFString _transitionType{std::vector<std::string>{"LINEAR"}};

  /**
   * @brief Member variable for type.
   */

  MFString _type{std::vector<std::string>{"EXAMINE", "ANY"}};

  /**
   * @brief Member variable for visibilityLimit.
   */

  SFFloat _visibilityLimit{0};
};

} // namespace x3d::nodes
