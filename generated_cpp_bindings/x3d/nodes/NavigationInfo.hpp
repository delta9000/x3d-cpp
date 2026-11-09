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
  NavigationInfo() = default;

  ~NavigationInfo() = default;

  static MFFloat getDefaultAvatarSize() {
    return std::vector<float>{0.25, 1.6, 0.75};
  }

  static SFBool getDefaultHeadlight() { return true; }

  static SFFloat getDefaultSpeed() { return 1; }

  static SFTime getDefaultTransitionTime() { return 1.0; }

  static MFString getDefaultTransitionType() {
    return std::vector<std::string>{"LINEAR"};
  }

  static MFString getDefaultType() {
    return std::vector<std::string>{"EXAMINE", "ANY"};
  }

  static SFFloat getDefaultVisibilityLimit() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 1; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details avatarSize triplet values define three separate parameters: (a)
   * collisionDistance between user and geometry, i.
   */
  const MFFloat &getAvatarSize() const { return _avatarSize; }

  /**
   * Access: inputOutput.
   * @details avatarSize triplet values define three separate parameters: (a)
   * collisionDistance between user and geometry, i.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAvatarSize() for range validation.
   */
  void setAvatarSizeUnchecked(const MFFloat &value) { _avatarSize = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enable/disable directional light that always points in the
   * direction the user is looking.
   */
  const SFBool &getHeadlight() const { return _headlight; }

  /**
   * Access: inputOutput.
   * @details Enable/disable directional light that always points in the
   * direction the user is looking.
   */
  void setHeadlight(const SFBool &value) { _headlight = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Default rate at which viewer travels through scene, meters/second.
   */
  const SFFloat &getSpeed() const { return _speed; }

  /**
   * Access: inputOutput.
   * @details Default rate at which viewer travels through scene, meters/second.
   */
  void setSpeed(const SFFloat &value) {

    validateSpeed(value);

    _speed = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSpeed() for range validation.
   */
  void setSpeedUnchecked(const SFFloat &value) { _speed = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Event signaling viewpoint transition complete.
   */
  const SFBool &getTransitionComplete() const { return _transitionComplete; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Event signaling viewpoint transition complete.
   */
  void emitTransitionComplete(const SFBool &value) {
    _transitionComplete = value;
  }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details transitionTime defines the expected duration of viewpoint
   * transition in seconds.
   */
  const SFTime &getTransitionTime() const { return _transitionTime; }

  /**
   * Access: inputOutput.
   * @details transitionTime defines the expected duration of viewpoint
   * transition in seconds.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTransitionTime() for range validation.
   */
  void setTransitionTimeUnchecked(const SFTime &value) {
    _transitionTime = value;
  }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Camera transition between viewpoints.
   */
  const MFString &getTransitionType() const { return _transitionType; }

  /**
   * Access: inputOutput.
   * @details Camera transition between viewpoints.
   */
  void setTransitionType(const MFString &value) { _transitionType = value; }

  void setTransitionType(MFString &&value) {

    _transitionType = std::move(value);
  }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Enter one or more quoted SFString values: "EXAMINE" "WALK" "FLY"
   * "LOOKAT" "EXPLORE" "ANY" "NONE".
   */
  const MFString &getType() const { return _type; }

  /**
   * Access: inputOutput.
   * @details Enter one or more quoted SFString values: "EXAMINE" "WALK" "FLY"
   * "LOOKAT" "EXPLORE" "ANY" "NONE".
   */
  void setType(const MFString &value) { _type = value; }

  void setType(MFString &&value) { _type = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Geometry beyond the visibilityLimit may not be rendered (far
   * clipping plane of the view frustrum).
   */
  const SFFloat &getVisibilityLimit() const { return _visibilityLimit; }

  /**
   * Access: inputOutput.
   * @details Geometry beyond the visibilityLimit may not be rendered (far
   * clipping plane of the view frustrum).
   */
  void setVisibilityLimit(const SFFloat &value) {

    validateVisibilityLimit(value);

    _visibilityLimit = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setVisibilityLimit() for range validation.
   */
  void setVisibilityLimitUnchecked(const SFFloat &value) {
    _visibilityLimit = value;
  }

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

  MFFloat _avatarSize{std::vector<float>{0.25, 1.6, 0.75}};

  SFBool _headlight{true};

  SFFloat _speed{1};

  SFBool _transitionComplete{};

  SFTime _transitionTime{1.0};

  MFString _transitionType{std::vector<std::string>{"LINEAR"}};

  MFString _type{std::vector<std::string>{"EXAMINE", "ANY"}};

  SFFloat _visibilityLimit{0};
};

} // namespace x3d::nodes
