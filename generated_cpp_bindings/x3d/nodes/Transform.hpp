// Transform.hpp
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

#include "x3d/nodes/X3DGroupingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Transform
 * @brief Transform is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#Transform
 */
class Transform : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for Transform
   */
  Transform() = default;

  /**
   * @brief Destructor for Transform
   */
  ~Transform() = default;

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
  static std::string getContainerFieldType() {
    return "containerFieldChoicesGroupLODShapeTransformSwitch";
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
  static std::string componentName() { return "Grouping"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

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
   * @brief The X3D type name of this node (e.g. "Transform").
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
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

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

} // namespace x3d::nodes
