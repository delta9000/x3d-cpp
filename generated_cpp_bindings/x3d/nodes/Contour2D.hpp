#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Contour2D
 * @brief Contour2D groups a set of curve segments into a composite contour.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#Contour2D
 */
class Contour2D : public virtual X3DNode {
public:
  Contour2D() = default;

  ~Contour2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "trimmingContour"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 4; }

  /**
   * @details Permitted X3D node types: NurbsCurve2D, ContourPolyline2D
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"NurbsCurve2D",
                                                   "ContourPolyline2D"};
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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: NurbsCurve2D, ContourPolyline2D
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"NurbsCurve2D",
                                                   "ContourPolyline2D"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @details Permitted X3D node types: NurbsCurve2D, ContourPolyline2D
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"NurbsCurve2D",
                                                   "ContourPolyline2D"};
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

  MFNode _children{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};
};

} // namespace x3d::nodes
