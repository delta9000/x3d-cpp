// HAnimJoint.hpp
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
  /**
   * @brief Default constructor for HAnimJoint
   */
  HAnimJoint() = default;

  /**
   * @brief Destructor for HAnimJoint
   */
  ~HAnimJoint() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for limitOrientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultLimitOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  /**
   * @brief Get the default value for llimit
   * @return MFFloat The default value
   */
  static MFFloat getDefaultLlimit() { return std::vector<float>{0, 0, 0}; }

  /**
   * @brief Get the default value for rotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for scale
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  /**
   * @brief Get the default value for scaleOrientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  /**
   * @brief Get the default value for stiffness
   * @return MFFloat The default value
   */
  static MFFloat getDefaultStiffness() { return std::vector<float>{0, 0, 0}; }

  /**
   * @brief Get the default value for translation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for ulimit
   * @return MFFloat The default value
   */
  static MFFloat getDefaultUlimit() { return std::vector<float>{0, 0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimJoint";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "HAnim"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Acceptable node types for the addChildren field.
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on addChildren.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnAddChildrenHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onAddChildren(const MFNode &value) {
    if (_onAddChildrenHandler)
      _onAddChildrenHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on addChildren.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnAddChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onAddChildrenHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details Grouping nodes contain an ordered list of children nodes.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * @brief Sets the value of children. AccessType: inputOutput
   * @details Grouping nodes contain an ordered list of children nodes.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

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
   * @brief Gets the value of displacers. AccessType: inputOutput
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimJoint object.
   * @return MFNode The current value of displacers.
   */
  MFNode getDisplacers() const { return _displacers; }

  /**
   * @brief Acceptable node types for the displacers field.
   * @details Permitted X3D node types: HAnimDisplacer
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableDisplacersNodeTypes() {
    static const std::vector<std::string> types = {"HAnimDisplacer"};
    return types;
  }

  /**
   * @brief Sets the value of displacers. AccessType: inputOutput
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimJoint object.
   * @param value The new value for displacers.
   */
  void setDisplacers(const MFNode &value) { _displacers = value; }

  void setDisplacers(MFNode &&value) { _displacers = std::move(value); }

  /**
   * @brief Gets the value of limitOrientation. AccessType: inputOutput
   * @details Orientation of upper/lower rotation limits, relative to HAnimJoint
   * center.
   * @return SFRotation The current value of limitOrientation.
   */
  SFRotation getLimitOrientation() const { return _limitOrientation; }

  /**
   * @brief Sets the value of limitOrientation. AccessType: inputOutput
   * @details Orientation of upper/lower rotation limits, relative to HAnimJoint
   * center.
   * @param value The new value for limitOrientation.
   */
  void setLimitOrientation(const SFRotation &value) {

    _limitOrientation = value;
  }

  void setLimitOrientation(SFRotation &&value) {

    _limitOrientation = std::move(value);
  }

  /**
   * @brief Gets the value of llimit. AccessType: inputOutput
   * @details Lower limit for minimum joint rotation in radians.
   * @return MFFloat The current value of llimit.
   */
  MFFloat getLlimit() const { return _llimit; }

  /**
   * @brief Sets the value of llimit. AccessType: inputOutput
   * @details Lower limit for minimum joint rotation in radians.
   * @param value The new value for llimit.
   */
  void setLlimit(const MFFloat &value) { _llimit = value; }

  void setLlimit(MFFloat &&value) { _llimit = std::move(value); }

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimJoint node can
   * be identified at run time for animation purposes.
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimJoint node can
   * be identified at run time for animation purposes.
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief Acceptable node types for the removeChildren field.
   * @details Permitted X3D node types: HAnimJoint, HAnimSegment
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint",
                                                   "HAnimSegment"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on removeChildren.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveChildrenHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onRemoveChildren(const MFNode &value) {
    if (_onRemoveChildrenHandler)
      _onRemoveChildrenHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * removeChildren.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnRemoveChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveChildrenHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system.
   * @return SFRotation The current value of rotation.
   */
  SFRotation getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system.
   * @param value The new value for rotation.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * @brief Gets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @return SFVec3f The current value of scale.
   */
  SFVec3f getScale() const { return _scale; }

  /**
   * @brief Sets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @param value The new value for scale.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * @brief Gets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @return SFRotation The current value of scaleOrientation.
   */
  SFRotation getScaleOrientation() const { return _scaleOrientation; }

  /**
   * @brief Sets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @param value The new value for scaleOrientation.
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * @brief Gets the value of skinCoordIndex. AccessType: inputOutput
   * @details Coordinate index values referencing which vertices are influenced
   * by the HAnimJoint.
   * @return MFInt32 The current value of skinCoordIndex.
   */
  MFInt32 getSkinCoordIndex() const { return _skinCoordIndex; }

  /**
   * @brief Sets the value of skinCoordIndex. AccessType: inputOutput
   * @details Coordinate index values referencing which vertices are influenced
   * by the HAnimJoint.
   * @param value The new value for skinCoordIndex.
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
   * @brief Non-validating write of skinCoordIndex (runtime/reader ingest path).
   * @details Assigns without the range check setSkinCoordIndex() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSkinCoordIndex() stays the
   *          enforcement point for programmatic callers.
   */
  void setSkinCoordIndexUnchecked(const MFInt32 &value) {
    _skinCoordIndex = value;
  }

  /**
   * @brief Gets the value of skinCoordWeight. AccessType: inputOutput
   * @details Weight deformation values for the corresponding values in the
   * skinCoordIndex field.
   * @return MFFloat The current value of skinCoordWeight.
   */
  MFFloat getSkinCoordWeight() const { return _skinCoordWeight; }

  /**
   * @brief Sets the value of skinCoordWeight. AccessType: inputOutput
   * @details Weight deformation values for the corresponding values in the
   * skinCoordIndex field.
   * @param value The new value for skinCoordWeight.
   */
  void setSkinCoordWeight(const MFFloat &value) { _skinCoordWeight = value; }

  void setSkinCoordWeight(MFFloat &&value) {

    _skinCoordWeight = std::move(value);
  }

  /**
   * @brief Gets the value of stiffness. AccessType: inputOutput
   * @details A scale factor of (1 - stiffness) is applied around the
   * corresponding axis (X, Y, or Z for entries 0, 1 and 2 of the stiffness
   * field).
   * @return MFFloat The current value of stiffness.
   */
  MFFloat getStiffness() const { return _stiffness; }

  /**
   * @brief Sets the value of stiffness. AccessType: inputOutput
   * @details A scale factor of (1 - stiffness) is applied around the
   * corresponding axis (X, Y, or Z for entries 0, 1 and 2 of the stiffness
   * field).
   * @param value The new value for stiffness.
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
   * @brief Non-validating write of stiffness (runtime/reader ingest path).
   * @details Assigns without the range check setStiffness() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setStiffness() stays the
   *          enforcement point for programmatic callers.
   */
  void setStiffnessUnchecked(const MFFloat &value) { _stiffness = value; }

  /**
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system.
   * @return SFVec3f The current value of translation.
   */
  SFVec3f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system.
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * @brief Gets the value of ulimit. AccessType: inputOutput
   * @details Upper limit for maximum joint rotation in radians.
   * @return MFFloat The current value of ulimit.
   */
  MFFloat getUlimit() const { return _ulimit; }

  /**
   * @brief Sets the value of ulimit. AccessType: inputOutput
   * @details Upper limit for maximum joint rotation in radians.
   * @param value The new value for ulimit.
   */
  void setUlimit(const MFFloat &value) { _ulimit = value; }

  void setUlimit(MFFloat &&value) { _ulimit = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "HAnimJoint").
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

      if (v < 0)
        throw std::out_of_range("stiffness below minimum of 0");
      if (v > 1)
        throw std::out_of_range("stiffness above maximum of 1");
    }
  }

  /**
   * @brief Registered event handler for addChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onAddChildrenHandler{};

  /**
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for displacers.
   */

  MFNode _displacers{};

  /**
   * @brief Member variable for limitOrientation.
   */

  SFRotation _limitOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for llimit.
   */

  MFFloat _llimit{std::vector<float>{0, 0, 0}};

  /**
   * @brief Member variable for name.
   */

  SFString _name{};

  /**
   * @brief Registered event handler for removeChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  /**
   * @brief Member variable for rotation.
   */

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for scale.
   */

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  /**
   * @brief Member variable for scaleOrientation.
   */

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for skinCoordIndex.
   */

  MFInt32 _skinCoordIndex{};

  /**
   * @brief Member variable for skinCoordWeight.
   */

  MFFloat _skinCoordWeight{};

  /**
   * @brief Member variable for stiffness.
   */

  MFFloat _stiffness{std::vector<float>{0, 0, 0}};

  /**
   * @brief Member variable for translation.
   */

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for ulimit.
   */

  MFFloat _ulimit{std::vector<float>{0, 0, 0}};
};

} // namespace x3d::nodes
