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
 * @class Circle2D
 * @brief Circle2D is a geometry node that defines a linear X-Y circle with
 * center (0,0) in X-Y plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Circle2D
 */
class Circle2D : public virtual X3DGeometryNode {
public:
  Circle2D() = default;

  ~Circle2D() = default;

  static SFFloat getDefaultRadius() { return 1; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details circle radius.
   */
  const SFFloat &getRadius() const { return _radius; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

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
  SFFloat _radius{1};
};

} // namespace x3d::nodes
