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

#include "x3d/nodes/X3DComposedGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class QuadSet
 * @brief QuadSet is a geometry node that defines quadrilaterals.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html#QuadSet
 */
class QuadSet : public virtual X3DComposedGeometryNode {
public:
  QuadSet() = default;

  ~QuadSet() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "CADGeometry"; }

  static int componentLevel() { return 1; }

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
