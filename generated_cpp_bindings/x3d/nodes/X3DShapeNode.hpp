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
 * @class X3DShapeNode
 * @brief Base type for all Shape nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#X3DShapeNode
 */
class X3DShapeNode : public virtual X3DChildNode,
                     public virtual X3DBoundedObject {
public:
  X3DShapeNode() = default;

  virtual ~X3DShapeNode() = default;

  static SFNode getDefaultAppearance() { return nullptr; }

  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultBboxDisplay() { return false; }

  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  static SFBool getDefaultCastShadow() { return true; }

  static SFNode getDefaultGeometry() { return nullptr; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getAppearance() const { return _appearance; }

  /**
   * @details Permitted X3D node types: X3DAppearanceNode
   */
  static const std::vector<std::string> &acceptableAppearanceNodeTypes() {
    static const std::vector<std::string> types = {"X3DAppearanceNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setAppearance(const SFNode &value) { _appearance = value; }

  void setAppearance(SFNode &&value) { _appearance = std::move(value); }

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
  const SFBool &getCastShadow() const { return _castShadow; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setCastShadow(const SFBool &value) { _castShadow = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getGeometry() const { return _geometry; }

  /**
   * @details Permitted X3D node types: X3DGeometryNode
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

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
  SFNode _appearance{nullptr};

  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  SFBool _bboxDisplay{false};

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  SFBool _castShadow{true};

  SFNode _geometry{nullptr};

  SFBool _visible{true};
};

} // namespace x3d::nodes
