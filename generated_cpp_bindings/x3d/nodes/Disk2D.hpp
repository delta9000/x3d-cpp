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
 * @class Disk2D
 * @brief Disk2D is a geometry node that defines a filled (or partially filled)
 * planar circle with center (0,0).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Disk2D
 */
class Disk2D : public virtual X3DGeometryNode {
public:
  Disk2D() = default;

  ~Disk2D() = default;

  static SFFloat getDefaultInnerRadius() { return 0; }

  static SFFloat getDefaultOuterRadius() { return 1; }

  static SFBool getDefaultSolid() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry2D"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Inner circle radius, greater than or equal to 0.
   */
  const SFFloat &getInnerRadius() const { return _innerRadius; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInnerRadiusUnchecked(const SFFloat &value) { _innerRadius = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Outer radius of circle, greater than or equal to inner radius.
   */
  const SFFloat &getOuterRadius() const { return _outerRadius; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setOuterRadiusUnchecked(const SFFloat &value) { _outerRadius = value; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesInnerRadius(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  SFFloat _innerRadius{0};

  SFFloat _outerRadius{1};

  SFBool _solid{false};
};

} // namespace x3d::nodes
