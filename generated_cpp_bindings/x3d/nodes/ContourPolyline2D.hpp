#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DNurbsControlCurveNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ContourPolyline2D
 * @brief ContourPolyline2D defines a linear curve segment as part of a trimming
 * contour in the u-v domain of a NURBS surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#ContourPolyline2D
 */
class ContourPolyline2D : public virtual X3DNurbsControlCurveNode {
public:
  ContourPolyline2D() = default;

  ~ContourPolyline2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 3; }

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
};

} // namespace x3d::nodes
