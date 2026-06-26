// CADPart.hpp
#ifndef CADPART_HPP
#define CADPART_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DProductStructureChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

/**
 * @class CADPart
 * @brief CADPart is an atomic part that defines both coordinate-system location
 * and the faces that constitute a part in a Computer-Aided Design (CAD) model.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html#CADPart
 */
class CADPart : public virtual X3DProductStructureChildNode,
                public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for CADPart
   */
  CADPart() = default;

  /**
   * @brief Destructor for CADPart
   */
  ~CADPart() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

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
   * @brief Get the default value for translation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

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
  static std::string componentName() { return "CADGeometry"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Acceptable node types for the addChildren field.
   * @details Permitted X3D node types: CADFace
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
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
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
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
   * @details Permitted X3D node types: CADFace
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
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
   * @brief Acceptable node types for the removeChildren field.
   * @details Permitted X3D node types: CADFace
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
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
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   * @return SFRotation The current value of rotation.
   */
  SFRotation getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
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
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   * @return SFVec3f The current value of translation.
   */
  SFVec3f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "CADPart").
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
   * @brief Member variable for translation.
   */

  SFVec3f _translation{SFVec3f{0, 0, 0}};
};

#endif // CADPART_HPP