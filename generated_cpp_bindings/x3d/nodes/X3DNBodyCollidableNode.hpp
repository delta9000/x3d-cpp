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
 * @class X3DNBodyCollidableNode
 * @brief The X3DNBodyCollidableNode abstract node type represents objects that
 * act as the interface between the rigid body physics, collision geometry
 * proxy, and renderable objects in the scene graph hierarchy.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#X3DNBodyCollidableNode
 */
class X3DNBodyCollidableNode : public virtual X3DChildNode,
                               public virtual X3DBoundedObject {
public:
  X3DNBodyCollidableNode() = default;

  virtual ~X3DNBodyCollidableNode() = default;

  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultBboxDisplay() { return false; }

  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  static SFBool getDefaultEnabled() { return true; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxCenter() const { return _bboxCenter; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getBboxDisplay() const { return _bboxDisplay; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxSize() const { return _bboxSize; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getVisible() const { return _visible; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVisible(const SFBool &value) { _visible = value; }

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
  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  SFBool _bboxDisplay{false};

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  SFBool _enabled{true};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  SFBool _visible{true};
};

} // namespace x3d::nodes
