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

#include "x3d/nodes/X3DProductStructureChildNode.hpp"

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DGroupingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

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
  CADPart() = default;

  ~CADPart() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "CADGeometry"; }

  static int componentLevel() { return 2; }

  /**
   * @details Permitted X3D node types: CADFace
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: CADFace
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @details Permitted X3D node types: CADFace
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"CADFace"};
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
  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onAddChildrenHandler{};

  SFVec3f _center{SFVec3f{0, 0, 0}};

  MFNode _children{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  SFVec3f _translation{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
