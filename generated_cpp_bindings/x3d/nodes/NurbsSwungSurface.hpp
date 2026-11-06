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
 * @class NurbsSwungSurface
 * @brief NurbsSwungSurface contains a profileCurve and a trajectoryCurve
 * [X3DNurbsControlCurveNode].
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSwungSurface
 */
class NurbsSwungSurface : public virtual X3DParametricGeometryNode {
public:
  NurbsSwungSurface() = default;

  ~NurbsSwungSurface() = default;

  static SFBool getDefaultCcw() { return true; }

  static SFNode getDefaultProfileCurve() { return nullptr; }

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
   * @details 2D curve in the yz-plane that describes the cross-sectional shape
   * of the object.
   */
  const SFNode &getProfileCurve() const { return _profileCurve; }

  /**
   * @details Permitted X3D node types: X3DNurbsControlCurveNode
   */
  static const std::vector<std::string> &acceptableProfileCurveNodeTypes() {
    static const std::vector<std::string> types = {"X3DNurbsControlCurveNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details 2D curve in the yz-plane that describes the cross-sectional shape
   * of the object.
   */
  void setProfileCurve(const SFNode &value) { _profileCurve = value; }

  void setProfileCurve(SFNode &&value) { _profileCurve = std::move(value); }

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
   * @details 2D curve in the xz-plane that describes path over which to trace
   * the cross-section.
   */
  const SFNode &getTrajectoryCurve() const { return _trajectoryCurve; }

  /**
   * @details Permitted X3D node types: X3DNurbsControlCurveNode
   */
  static const std::vector<std::string> &acceptableTrajectoryCurveNodeTypes() {
    static const std::vector<std::string> types = {"X3DNurbsControlCurveNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details 2D curve in the xz-plane that describes path over which to trace
   * the cross-section.
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

  SFNode _profileCurve{nullptr};

  SFBool _solid{true};

  SFNode _trajectoryCurve{nullptr};
};

} // namespace x3d::nodes
