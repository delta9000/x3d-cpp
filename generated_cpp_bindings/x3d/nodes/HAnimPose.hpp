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
 * @class HAnimPose
 * @brief HAnimPose nodes support setting HAnimJoint values in a corresponding
 * HAnimHumanoid skeleton.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/hanim.html#HAnimPose
 */
class HAnimPose : public virtual X3DChildNode {
public:
  HAnimPose() = default;

  ~HAnimPose() = default;

  static SFBool getDefaultEnabled() { return true; }

  static SFInt32 getDefaultLoa() { return -1; }

  static SFTime getDefaultTransitionDuration() { return 0.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children field lists all HAnimJoint nodes making up this pose.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: HAnimJoint
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children field lists all HAnimJoint nodes making up this pose.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFBool; access: inputOnly.
   * @details commencePose is an input boolean event that triggers the animation
   * of Joint values to achieve the pose. Dispatches to the handler registered
   * via setOnCommencePoseHandler(); a no-op if none is set.
   */
  void onCommencePose(const SFBool &value) {
    if (_onCommencePoseHandler)
      _onCommencePoseHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnCommencePoseHandler(std::function<void(const SFBool &)> handler) {
    _onCommencePoseHandler = std::move(handler);
  }

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
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details isActive true/false events are sent when pose animation
   * starts/stops.
   */
  const SFBool &getIsActive() const { return _isActive; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details isActive true/false events are sent when pose animation
   * starts/stops.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

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
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that HAnimPose node can
   * be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that HAnimPose node can
   * be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * SFBool; access: inputOnly.
   * @details resetAllJoints is an input boolean event that zeroes all Joint
   * objects in the skeleton field to their default values, matching the default
   * binding pose (i. Dispatches to the handler registered via
   * setOnResetAllJointsHandler(); a no-op if none is set.
   */
  void onResetAllJoints(const SFBool &value) {
    if (_onResetAllJointsHandler)
      _onResetAllJointsHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnResetAllJointsHandler(std::function<void(const SFBool &)> handler) {
    _onResetAllJointsHandler = std::move(handler);
  }

  /**
   * SFTime; access: inputOnly.
   * @details setting fraction in range [0,1] provides an input event that
   * allows gradual change of each joint in the pose, where 0 indicates no
   * change and 1 indicates fully changed. Dispatches to the handler registered
   * via setOnSet_fractionHandler(); a no-op if none is set.
   */
  void onSet_fraction(const SFTime &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_fractionHandler(std::function<void(const SFTime &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

  /**
   * SFTime; access: inputOnly.
   * @details set_startTime is an input event that commences the animation of
   * Joint objects in the parent Humanoid object. Dispatches to the handler
   * registered via setOnSet_startTimeHandler(); a no-op if none is set.
   */
  void onSet_startTime(const SFTime &value) {
    if (_onSet_startTimeHandler)
      _onSet_startTimeHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_startTimeHandler(std::function<void(const SFTime &)> handler) {
    _onSet_startTimeHandler = std::move(handler);
  }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Duration in seconds for animating a pose transition.
   */
  const SFTime &getTransitionDuration() const { return _transitionDuration; }

  /**
   * Access: inputOutput.
   * @details Duration in seconds for animating a pose transition.
   */
  void setTransitionDuration(const SFTime &value) {

    validateTransitionDuration(value);

    _transitionDuration = value;
  }

  void setTransitionDuration(SFTime &&value) {

    validateTransitionDuration(value);

    _transitionDuration = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTransitionDuration() for range validation.
   */
  void setTransitionDurationUnchecked(const SFTime &value) {
    _transitionDuration = value;
  }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

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
  static void checkRangesLoa(const SFInt32 &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTransitionDuration(const SFTime &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out);

private:
  static void validateLoa(const SFInt32 &value) {

    if (value < -1)
      throw std::out_of_range("loa below minimum of -1");
    if (value > 5)
      throw std::out_of_range("loa above maximum of 5");
  }

  static void validateTransitionDuration(const SFTime &value) {

    if (value < 0.0)
      throw std::out_of_range("transitionDuration below minimum of 0.0");
  }

  MFNode _children{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onCommencePoseHandler{};

  SFString _description{};

  SFBool _enabled{true};

  SFBool _isActive{};

  SFInt32 _loa{-1};

  SFString _name{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onResetAllJointsHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFTime &)> _onSet_fractionHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFTime &)> _onSet_startTimeHandler{};

  SFTime _transitionDuration{0.0};
};

} // namespace x3d::nodes
