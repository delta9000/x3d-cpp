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
 * @class X3DNurbsControlCurveNode
 * @brief Base type for all nodes that provide control curve information in 2D
 * space.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#X3DNurbsControlCurveNode
 */
class X3DNurbsControlCurveNode : public virtual X3DNode {
public:
  X3DNurbsControlCurveNode() = default;

  virtual ~X3DNurbsControlCurveNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec2d; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFVec2d &getControlPoint() const { return _controlPoint; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setControlPoint(const MFVec2d &value) { _controlPoint = value; }

  void setControlPoint(MFVec2d &&value) { _controlPoint = std::move(value); }

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
  MFVec2d _controlPoint{};
};

} // namespace x3d::nodes
