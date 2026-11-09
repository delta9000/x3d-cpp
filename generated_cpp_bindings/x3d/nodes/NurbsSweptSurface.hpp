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

#include "x3d/nodes/X3DParametricGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsSweptSurface
 * @brief NurbsSweptSurface uses a trajectoryCurve path to describe a
 * generalized surface that is swept by a crossSectionCurve.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSweptSurface
 */
class NurbsSweptSurface : public virtual X3DParametricGeometryNode {
public:
  NurbsSweptSurface() = default;

  ~NurbsSweptSurface() = default;

  static SFBool getDefaultCcw() { return true; }

  static SFNode getDefaultCrossSectionCurve() { return nullptr; }

  static SFBool getDefaultSolid() { return true; }

  static SFNode getDefaultTrajectoryCurve() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 3; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details ccw defines clockwise/counterclockwise ordering of vertex
   * coordinates, which in turn defines front/back orientation of polygon
   * normals according to Right-Hand Rule (RHR).
   */
  const SFBool &getCcw() const { return _ccw; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details defines cross-section of the surface traced about the
   * trajectoryCurve axis.
   */
  const SFNode &getCrossSectionCurve() const { return _crossSectionCurve; }

  /**
   * @details Permitted X3D node types: X3DNurbsControlCurveNode
   */
  static const std::vector<std::string> &
  acceptableCrossSectionCurveNodeTypes() {
    static const std::vector<std::string> types = {"X3DNurbsControlCurveNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details defines cross-section of the surface traced about the
   * trajectoryCurve axis.
   */
  void setCrossSectionCurve(const SFNode &value) { _crossSectionCurve = value; }

  void setCrossSectionCurve(SFNode &&value) {

    _crossSectionCurve = std::move(value);
  }

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
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details describes the center-line path using a NurbsCurve node, oriented
   * so that it is defined counterclockwise when looking down the −Y axis, thus
   * defining a concept of inside and outside.
   */
  const SFNode &getTrajectoryCurve() const { return _trajectoryCurve; }

  /**
   * @details Permitted X3D node types: NurbsCurve
   */
  static const std::vector<std::string> &acceptableTrajectoryCurveNodeTypes() {
    static const std::vector<std::string> types = {"NurbsCurve"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details describes the center-line path using a NurbsCurve node, oriented
   * so that it is defined counterclockwise when looking down the −Y axis, thus
   * defining a concept of inside and outside.
   */
  void setTrajectoryCurve(const SFNode &value) { _trajectoryCurve = value; }

  void setTrajectoryCurve(SFNode &&value) {

    _trajectoryCurve = std::move(value);
  }

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
  SFBool _ccw{true};

  SFNode _crossSectionCurve{nullptr};

  SFBool _solid{true};

  SFNode _trajectoryCurve{nullptr};
};

} // namespace x3d::nodes
