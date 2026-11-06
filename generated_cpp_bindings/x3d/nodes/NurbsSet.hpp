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
 * @class NurbsSet
 * @brief NurbsSet collects a set of NurbsSurface nodes into a common group and
 * treats NurbsSurface set as a unit during tessellation, thereby enforcing
 * tessellation continuity along borders.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSet
 */
class NurbsSet : public virtual X3DChildNode, public virtual X3DBoundedObject {
public:
  NurbsSet() = default;

  ~NurbsSet() = default;

  static SFFloat getDefaultTessellationScale() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 2; }

  /**
   * @details Permitted X3D node types: X3DParametricGeometryNode
   */
  static const std::vector<std::string> &acceptableAddGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnAddGeometryHandler(); a no-op if none is set.
   */
  void onAddGeometry(const MFNode &value) {
    if (_onAddGeometryHandler)
      _onAddGeometryHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnAddGeometryHandler(std::function<void(const MFNode &)> handler) {
    _onAddGeometryHandler = std::move(handler);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   */
  const MFNode &getGeometry() const { return _geometry; }

  /**
   * @details Permitted X3D node types: X3DParametricGeometryNode
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   */
  void setGeometry(const MFNode &value) { _geometry = value; }

  void setGeometry(MFNode &&value) { _geometry = std::move(value); }

  /**
   * @details Permitted X3D node types: X3DParametricGeometryNode
   */
  static const std::vector<std::string> &acceptableRemoveGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveGeometryHandler(); a no-op if none is set.
   */
  void onRemoveGeometry(const MFNode &value) {
    if (_onRemoveGeometryHandler)
      _onRemoveGeometryHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnRemoveGeometryHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveGeometryHandler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details scale for surface tessellation in children NurbsSurface nodes.
   */
  const SFFloat &getTessellationScale() const { return _tessellationScale; }

  /**
   * Access: inputOutput.
   * @details scale for surface tessellation in children NurbsSurface nodes.
   */
  void setTessellationScale(const SFFloat &value) {

    _tessellationScale = value;
  }

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
  std::function<void(const MFNode &)> _onAddGeometryHandler{};

  MFNode _geometry{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveGeometryHandler{};

  SFFloat _tessellationScale{1.0};
};

} // namespace x3d::nodes
