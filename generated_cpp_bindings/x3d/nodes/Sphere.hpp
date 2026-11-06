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
 * @class Sphere
 * @brief Sphere is a geometry node, representing a perfectly round geometrical
 * object that is the surface of a completely round ball.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#Sphere
 */
class Sphere : public virtual X3DGeometryNode {
public:
  Sphere() = default;

  ~Sphere() = default;

  static SFFloat getDefaultRadius() { return 1; }

  static SFBool getDefaultSolid() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry3D"; }

  static int componentLevel() { return 1; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Size in meters.
   */
  const SFFloat &getRadius() const { return _radius; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

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
  SFFloat _radius{1};

  SFBool _solid{true};
};

} // namespace x3d::nodes
