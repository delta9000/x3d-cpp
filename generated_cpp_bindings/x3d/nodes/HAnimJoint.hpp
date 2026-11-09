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

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimJoint
 * @brief HAnimJoint node can represent each joint in a body.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimJoint
 */
class HAnimJoint : public virtual X3DChildNode,
                   public virtual X3DBoundedObject {
public:
  HAnimJoint() = default;

  ~HAnimJoint() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFRotation getDefaultLimitOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static MFFloat getDefaultLlimit() { return std::vector<float>{0, 0, 0}; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static MFFloat getDefaultStiffness() { return std::vector<float>{0, 0, 0}; }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static MFFloat getDefaultUlimit() { return std::vector<float>{0, 0, 0}; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimJoint";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 1; }

  /**
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnAddChildrenHandler(); a no-op if none is set.
   */
  void onAddChildren(const MFNode &value) {
    if (_onAddChildrenHandler)
      _onAddChildrenHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnAddChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onAddChildrenHandler = std::move(handler);
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimJoint object.
   */
  const MFNode &getDisplacers() const { return _displacers; }

  /**
   * @details Permitted X3D node types: HAnimDisplacer
   */
  static const std::vector<std::string> &acceptableDisplacersNodeTypes() {
    static const std::vector<std::string> types = {"HAnimDisplacer"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimJoint object.
   */
  void setDisplacers(const MFNode &value) { _displacers = value; }

  void setDisplacers(MFNode &&value) { _displacers = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation of upper/lower rotation limits, relative to HAnimJoint
   * center.
   */
  const SFRotation &getLimitOrientation() const { return _limitOrientation; }

  /**
   * Access: inputOutput.
   * @details Orientation of upper/lower rotation limits, relative to HAnimJoint
   * center.
   */
  void setLimitOrientation(const SFRotation &value) {

    _limitOrientation = value;
  }

  void setLimitOrientation(SFRotation &&value) {

    _limitOrientation = std::move(value);
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Lower limit for minimum joint rotation in radians.
   */
  const MFFloat &getLlimit() const { return _llimit; }

  /**
   * Access: inputOutput.
   * @details Lower limit for minimum joint rotation in radians.
   */
  void setLlimit(const MFFloat &value) { _llimit = value; }

  void setLlimit(MFFloat &&value) { _llimit = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that HAnimJoint node can
   * be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that HAnimJoint node can
   * be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveChildrenHandler(); a no-op if none is set.
   */
  void onRemoveChildren(const MFNode &value) {
    if (_onRemoveChildrenHandler)
      _onRemoveChildrenHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnRemoveChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveChildrenHandler = std::move(handler);
  }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation of children relative to local coordinate system.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details Orientation of children relative to local coordinate system.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  const SFVec3f &getScale() const { return _scale; }

  /**
   * Access: inputOutput.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  const SFRotation &getScaleOrientation() const { return _scaleOrientation; }

  /**
   * Access: inputOutput.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Coordinate index values referencing which vertices are influenced
   * by the HAnimJoint.
   */
  const MFInt32 &getSkinCoordIndex() const { return _skinCoordIndex; }

  /**
   * Access: inputOutput.
   * @details Coordinate index values referencing which vertices are influenced
   * by the HAnimJoint.
   */
  void setSkinCoordIndex(const MFInt32 &value) {

    validateSkinCoordIndex(value);

    _skinCoordIndex = value;
  }

  void setSkinCoordIndex(MFInt32 &&value) {

    validateSkinCoordIndex(value);

    _skinCoordIndex = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSkinCoordIndex() for range validation.
   */
  void setSkinCoordIndexUnchecked(const MFInt32 &value) {
    _skinCoordIndex = value;
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Weight deformation values for the corresponding values in the
   * skinCoordIndex field.
   */
  const MFFloat &getSkinCoordWeight() const { return _skinCoordWeight; }

  /**
   * Access: inputOutput.
   * @details Weight deformation values for the corresponding values in the
   * skinCoordIndex field.
   */
  void setSkinCoordWeight(const MFFloat &value) { _skinCoordWeight = value; }

  void setSkinCoordWeight(MFFloat &&value) {

    _skinCoordWeight = std::move(value);
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details A scale factor of (1 - stiffness) is applied around the
   * corresponding axis (X, Y, or Z for entries 0, 1 and 2 of the stiffness
   * field).
   */
  const MFFloat &getStiffness() const { return _stiffness; }

  /**
   * Access: inputOutput.
   * @details A scale factor of (1 - stiffness) is applied around the
   * corresponding axis (X, Y, or Z for entries 0, 1 and 2 of the stiffness
   * field).
   */
  void setStiffness(const MFFloat &value) {

    validateStiffness(value);

    _stiffness = value;
  }

  void setStiffness(MFFloat &&value) {

    validateStiffness(value);

    _stiffness = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStiffness() for range validation.
   */
  void setStiffnessUnchecked(const MFFloat &value) { _stiffness = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position of children relative to local coordinate system.
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details Position of children relative to local coordinate system.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Upper limit for maximum joint rotation in radians.
   */
  const MFFloat &getUlimit() const { return _ulimit; }

  /**
   * Access: inputOutput.
   * @details Upper limit for maximum joint rotation in radians.
   */
  void setUlimit(const MFFloat &value) { _ulimit = value; }

  void setUlimit(MFFloat &&value) { _ulimit = std::move(value); }

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
  static void checkRangesSkinCoordIndex(const MFInt32 &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStiffness(const MFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateSkinCoordIndex(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 0)
        throw std::out_of_range("skinCoordIndex below minimum of 0");
    }
  }

  static void validateStiffness(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("stiffness below minimum of 0");
      if (v > 1.0f)
        throw std::out_of_range("stiffness above maximum of 1");
    }
  }

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onAddChildrenHandler{};

  SFVec3f _center{SFVec3f{0, 0, 0}};

  MFNode _children{};

  SFString _description{};

  MFNode _displacers{};

  SFRotation _limitOrientation{SFRotation{0, 0, 1, 0}};

  MFFloat _llimit{std::vector<float>{0, 0, 0}};

  SFString _name{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  MFInt32 _skinCoordIndex{};

  MFFloat _skinCoordWeight{};

  MFFloat _stiffness{std::vector<float>{0, 0, 0}};

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  MFFloat _ulimit{std::vector<float>{0, 0, 0}};
};

} // namespace x3d::nodes
