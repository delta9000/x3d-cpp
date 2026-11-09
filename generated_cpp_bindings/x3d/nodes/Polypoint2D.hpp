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
 * @class Polypoint2D
 * @brief Polypoint2D is a geometry node that defines a set of 2D points in X-Y
 * plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Polypoint2D
 */
class Polypoint2D : public virtual X3DGeometryNode {
public:
  Polypoint2D() = default;

  ~Polypoint2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec2f; access: inputOutput. Borrows the stored value.
   * @details 2D coordinates of vertices.
   */
  const MFVec2f &getPoint() const { return _point; }

  /**
   * Access: inputOutput.
   * @details 2D coordinates of vertices.
   */
  void setPoint(const MFVec2f &value) { _point = value; }

  void setPoint(MFVec2f &&value) { _point = std::move(value); }

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
  MFVec2f _point{};
};

} // namespace x3d::nodes
