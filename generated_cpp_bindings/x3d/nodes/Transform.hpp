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
  Transform() = default;

  ~Transform() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesGroupLODShapeTransformSwitch";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Grouping"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
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
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

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
  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  SFVec3f _translation{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
