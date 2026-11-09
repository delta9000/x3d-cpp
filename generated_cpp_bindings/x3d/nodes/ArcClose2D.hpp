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
 * @class ArcClose2D
 * @brief ArcClose2D is a polygonal geometry node that defines a linear circular
 * arc, closed by PIE or CHORD line segments, with center (0,0) in X-Y plane,
 * with angles measured starting at positive x-axis and sweeping towards
 * positive y-axis.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#ArcClose2D
 */
class ArcClose2D : public virtual X3DGeometryNode {
public:
  ArcClose2D() = default;

  ~ArcClose2D() = default;

  static ClosureTypeChoices getDefaultClosureType() {
    return ClosureTypeChoices::PIE;
  }

  static SFFloat getDefaultEndAngle() { return 1.570796; }

  static SFFloat getDefaultRadius() { return 1; }

  static SFBool getDefaultSolid() { return false; }

  static SFFloat getDefaultStartAngle() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 2; }

  /**
   * ClosureTypeChoices; access: initializeOnly. Borrows the stored value.
   * @details Defines whether pair of line segments connect to center (PIE), or
   * single line-segment chord connects arc endpoints (CHORD).
   */
  const ClosureTypeChoices &getClosureType() const { return _closureType; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setClosureTypeUnchecked(const ClosureTypeChoices &value) {
    _closureType = value;
  }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Arc extends from startAngle counterclockwise to endAngle, in
   * radians.
   */
  const SFFloat &getEndAngle() const { return _endAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setEndAngleUnchecked(const SFFloat &value) { _endAngle = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details circle radius, of which the arc is a portion.
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

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Arc extends from startAngle counterclockwise to endAngle, in
   * radians.
   */
  const SFFloat &getStartAngle() const { return _startAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setStartAngleUnchecked(const SFFloat &value) { _startAngle = value; }

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
  ClosureTypeChoices _closureType{ClosureTypeChoices::PIE};

  SFFloat _endAngle{1.570796};

  SFFloat _radius{1};

  SFBool _solid{false};

  SFFloat _startAngle{0};
};

} // namespace x3d::nodes
