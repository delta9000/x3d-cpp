#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DPickableObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DLayerNode
 * @brief The X3DLayerNode abstract node type is the base node type for layer
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#X3DLayerNode
 */
class X3DLayerNode : public virtual X3DNode, public virtual X3DPickableObject {
public:
  X3DLayerNode() = default;

  virtual ~X3DLayerNode() = default;

  static MFString getDefaultObjectType() {
    return std::vector<std::string>{"ALL"};
  }

  static SFBool getDefaultPickable() { return true; }

  static SFNode getDefaultViewport() { return nullptr; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Layering"; }

  static int componentLevel() { return 1; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFString &getObjectType() const { return _objectType; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setObjectType(const MFString &value) { _objectType = value; }

  void setObjectType(MFString &&value) { _objectType = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getPickable() const { return _pickable; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setPickable(const SFBool &value) { _pickable = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getViewport() const { return _viewport; }

  /**
   * @details Permitted X3D node types: X3DViewportNode
   */
  static const std::vector<std::string> &acceptableViewportNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewportNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setViewport(const SFNode &value) { _viewport = value; }

  void setViewport(SFNode &&value) { _viewport = std::move(value); }

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
  MFString _objectType{std::vector<std::string>{"ALL"}};

  SFBool _pickable{true};

  SFNode _viewport{nullptr};

  SFBool _visible{true};
};

} // namespace x3d::nodes
