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
 * @class Rectangle2D
 * @brief Rectangle2D is a geometry node that defines a 2D rectangle in X-Y
 * plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Rectangle2D
 */
class Rectangle2D : public virtual X3DGeometryNode {
public:
  Rectangle2D() = default;

  ~Rectangle2D() = default;

  static SFVec2f getDefaultSize() { return SFVec2f{2, 2}; }

  static SFBool getDefaultSolid() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec2f; access: initializeOnly. Borrows the stored value.
   * @details 2D dimensions of Rectangle2D.
   */
  const SFVec2f &getSize() const { return _size; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSizeUnchecked(const SFVec2f &value) { _size = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

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
  SFVec2f _size{SFVec2f{2, 2}};

  SFBool _solid{false};
};

} // namespace x3d::nodes
