#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Polyline2D
 * @brief Polyline2D is a geometry node that defines a connected set of vertices
 * in a contiguous set of line segments in X-Y plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Polyline2D
 */
class Polyline2D : public virtual X3DGeometryNode {
public:
  Polyline2D() = default;

  ~Polyline2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec2f; access: initializeOnly. Borrows the stored value.
   * @details Coordinates of vertices connected into contiguous Polyline2D.
   */
  const MFVec2f &getLineSegments() const { return _lineSegments; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setLineSegmentsUnchecked(const MFVec2f &value) { _lineSegments = value; }

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
  MFVec2f _lineSegments{};
};

} // namespace x3d::nodes
