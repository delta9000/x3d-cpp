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
 * @class Extrusion
 * @brief Extrusion is a geometry node that sequentially stretches a 2D cross
 * section along a 3D-spine path in the local coordinate system, creating an
 * outer hull.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#Extrusion
 */
class Extrusion : public virtual X3DGeometryNode {
public:
  Extrusion() = default;

  ~Extrusion() = default;

  static SFBool getDefaultBeginCap() { return true; }

  static SFBool getDefaultCcw() { return true; }

  static SFBool getDefaultConvex() { return true; }

  static SFFloat getDefaultCreaseAngle() { return 0; }

  static MFVec2f getDefaultCrossSection() {
    return std::vector<SFVec2f>{SFVec2f{1, 1}, SFVec2f{1, -1}, SFVec2f{-1, -1},
                                SFVec2f{-1, 1}, SFVec2f{1, 1}};
  }

  static SFBool getDefaultEndCap() { return true; }

  static MFRotation getDefaultOrientation() {
    return std::vector<SFRotation>{SFRotation{0, 0, 1, 0}};
  }

  static MFVec2f getDefaultScale() {
    return std::vector<SFVec2f>{SFVec2f{1, 1}};
  }

  static SFBool getDefaultSolid() { return true; }

  static MFVec3f getDefaultSpine() {
    return std::vector<SFVec3f>{SFVec3f{0, 0, 0}, SFVec3f{0, 1, 0}};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry3D"; }

  static int componentLevel() { return 4; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether beginning cap is drawn (similar to Cylinder top cap).
   */
  const SFBool &getBeginCap() const { return _beginCap; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBeginCapUnchecked(const SFBool &value) { _beginCap = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details The ccw field indicates counterclockwise ordering of
   * vertex-coordinates orientation.
   */
  const SFBool &getCcw() const { return _ccw; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details The convex field is a hint to renderers whether all polygons in a
   * shape are convex (true), or possibly concave (false).
   */
  const SFBool &getConvex() const { return _convex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setConvexUnchecked(const SFBool &value) { _convex = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details creaseAngle defines angle (in radians) where adjacent polygons are
   * drawn with sharp edges or smooth shading.
   */
  const SFFloat &getCreaseAngle() const { return _creaseAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * MFVec2f; access: initializeOnly. Borrows the stored value.
   * @details The crossSection array defines a silhouette outline of the outer
   * Extrusion surface.
   */
  const MFVec2f &getCrossSection() const { return _crossSection; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCrossSectionUnchecked(const MFVec2f &value) { _crossSection = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether end cap is drawn (similar to Cylinder bottom cap).
   */
  const SFBool &getEndCap() const { return _endCap; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setEndCapUnchecked(const SFBool &value) { _endCap = value; }

  /**
   * MFRotation; access: initializeOnly. Borrows the stored value.
   * @details The orientation array is a list of axis-angle 4-tuple values
   * applied at each spine-aligned cross-section plane.
   */
  const MFRotation &getOrientation() const { return _orientation; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setOrientationUnchecked(const MFRotation &value) {
    _orientation = value;
  }

  /**
   * MFVec2f; access: initializeOnly. Borrows the stored value.
   * @details scale is a list of 2D-scale parameters applied at each
   * spine-aligned cross-section plane.
   */
  const MFVec2f &getScale() const { return _scale; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setScaleUnchecked(const MFVec2f &value) { _scale = value; }

  /**
   * MFVec2f; access: inputOnly.
   * @details The crossSection array defines a silhouette outline of the outer
   * Extrusion surface. Dispatches to the handler registered via
   * setOnSet_crossSectionHandler(); a no-op if none is set.
   */
  void onSet_crossSection(const MFVec2f &value) {
    if (_onSet_crossSectionHandler)
      _onSet_crossSectionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_crossSectionHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_crossSectionHandler = std::move(handler);
  }

  /**
   * MFRotation; access: inputOnly.
   * @details The orientation array is a list of axis-angle 4-tuple values
   * applied at each spine-aligned cross-section plane. Dispatches to the
   * handler registered via setOnSet_orientationHandler(); a no-op if none is
   * set.
   */
  void onSet_orientation(const MFRotation &value) {
    if (_onSet_orientationHandler)
      _onSet_orientationHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_orientationHandler(std::function<void(const MFRotation &)> handler) {
    _onSet_orientationHandler = std::move(handler);
  }

  /**
   * MFVec2f; access: inputOnly.
   * @details scale is a list of 2D-scale parameters applied at each
   * spine-aligned cross-section plane. Dispatches to the handler registered via
   * setOnSet_scaleHandler(); a no-op if none is set.
   */
  void onSet_scale(const MFVec2f &value) {
    if (_onSet_scaleHandler)
      _onSet_scaleHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_scaleHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_scaleHandler = std::move(handler);
  }

  /**
   * MFVec3f; access: inputOnly.
   * @details The spine array defines a center-line sequence of 3D points that
   * define a piecewise-linear curve forming a series of connected vertices.
   *          Dispatches to the handler registered via setOnSet_spineHandler();
   *          a no-op if none is set.
   */
  void onSet_spine(const MFVec3f &value) {
    if (_onSet_spineHandler)
      _onSet_spineHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_spineHandler(std::function<void(const MFVec3f &)> handler) {
    _onSet_spineHandler = std::move(handler);
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
   * MFVec3f; access: initializeOnly. Borrows the stored value.
   * @details The spine array defines a center-line sequence of 3D points that
   * define a piecewise-linear curve forming a series of connected vertices.
   */
  const MFVec3f &getSpine() const { return _spine; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSpineUnchecked(const MFVec3f &value) { _spine = value; }

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
  static void checkRangesCreaseAngle(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  SFBool _beginCap{true};

  SFBool _ccw{true};

  SFBool _convex{true};

  SFFloat _creaseAngle{0};

  MFVec2f _crossSection{std::vector<SFVec2f>{SFVec2f{1, 1}, SFVec2f{1, -1},
                                             SFVec2f{-1, -1}, SFVec2f{-1, 1},
                                             SFVec2f{1, 1}}};

  SFBool _endCap{true};

  MFRotation _orientation{std::vector<SFRotation>{SFRotation{0, 0, 1, 0}}};

  MFVec2f _scale{std::vector<SFVec2f>{SFVec2f{1, 1}}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFVec2f &)> _onSet_crossSectionHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFRotation &)> _onSet_orientationHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFVec2f &)> _onSet_scaleHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFVec3f &)> _onSet_spineHandler{};

  SFBool _solid{true};

  MFVec3f _spine{std::vector<SFVec3f>{SFVec3f{0, 0, 0}, SFVec3f{0, 1, 0}}};
};

} // namespace x3d::nodes
