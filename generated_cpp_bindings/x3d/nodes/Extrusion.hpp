// Extrusion.hpp
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
  /**
   * @brief Default constructor for Extrusion
   */
  Extrusion() = default;

  /**
   * @brief Destructor for Extrusion
   */
  ~Extrusion() = default;

  /**
   * @brief Get the default value for beginCap
   * @return SFBool The default value
   */
  static SFBool getDefaultBeginCap() { return true; }

  /**
   * @brief Get the default value for ccw
   * @return SFBool The default value
   */
  static SFBool getDefaultCcw() { return true; }

  /**
   * @brief Get the default value for convex
   * @return SFBool The default value
   */
  static SFBool getDefaultConvex() { return true; }

  /**
   * @brief Get the default value for creaseAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultCreaseAngle() { return 0; }

  /**
   * @brief Get the default value for crossSection
   * @return MFVec2f The default value
   */
  static MFVec2f getDefaultCrossSection() {
    return std::vector<SFVec2f>{SFVec2f{1, 1}, SFVec2f{1, -1}, SFVec2f{-1, -1},
                                SFVec2f{-1, 1}, SFVec2f{1, 1}};
  }

  /**
   * @brief Get the default value for endCap
   * @return SFBool The default value
   */
  static SFBool getDefaultEndCap() { return true; }

  /**
   * @brief Get the default value for orientation
   * @return MFRotation The default value
   */
  static MFRotation getDefaultOrientation() {
    return std::vector<SFRotation>{SFRotation{0, 0, 1, 0}};
  }

  /**
   * @brief Get the default value for scale
   * @return MFVec2f The default value
   */
  static MFVec2f getDefaultScale() {
    return std::vector<SFVec2f>{SFVec2f{1, 1}};
  }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

  /**
   * @brief Get the default value for spine
   * @return MFVec3f The default value
   */
  static MFVec3f getDefaultSpine() {
    return std::vector<SFVec3f>{SFVec3f{0, 0, 0}, SFVec3f{0, 1, 0}};
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Geometry3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of beginCap. AccessType: initializeOnly
   * @details Whether beginning cap is drawn (similar to Cylinder top cap).
   * @return SFBool The current value of beginCap.
   */
  SFBool getBeginCap() const { return _beginCap; }
  /**
   * @brief Data-layer write of beginCap (reader/init ingest path).
   * @details beginCap is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBeginCap().
   */
  void setBeginCapUnchecked(const SFBool &value) { _beginCap = value; }

  /**
   * @brief Gets the value of ccw. AccessType: initializeOnly
   * @details The ccw field indicates counterclockwise ordering of
   * vertex-coordinates orientation.
   * @return SFBool The current value of ccw.
   */
  SFBool getCcw() const { return _ccw; }
  /**
   * @brief Data-layer write of ccw (reader/init ingest path).
   * @details ccw is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCcw().
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * @brief Gets the value of convex. AccessType: initializeOnly
   * @details The convex field is a hint to renderers whether all polygons in a
   * shape are convex (true), or possibly concave (false).
   * @return SFBool The current value of convex.
   */
  SFBool getConvex() const { return _convex; }
  /**
   * @brief Data-layer write of convex (reader/init ingest path).
   * @details convex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setConvex().
   */
  void setConvexUnchecked(const SFBool &value) { _convex = value; }

  /**
   * @brief Gets the value of creaseAngle. AccessType: initializeOnly
   * @details creaseAngle defines angle (in radians) where adjacent polygons are
   * drawn with sharp edges or smooth shading.
   * @return SFFloat The current value of creaseAngle.
   */
  SFFloat getCreaseAngle() const { return _creaseAngle; }
  /**
   * @brief Data-layer write of creaseAngle (reader/init ingest path).
   * @details creaseAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCreaseAngle().
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * @brief Gets the value of crossSection. AccessType: initializeOnly
   * @details The crossSection array defines a silhouette outline of the outer
   * Extrusion surface.
   * @return MFVec2f The current value of crossSection.
   */
  MFVec2f getCrossSection() const { return _crossSection; }
  /**
   * @brief Data-layer write of crossSection (reader/init ingest path).
   * @details crossSection is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCrossSection().
   */
  void setCrossSectionUnchecked(const MFVec2f &value) { _crossSection = value; }

  /**
   * @brief Gets the value of endCap. AccessType: initializeOnly
   * @details Whether end cap is drawn (similar to Cylinder bottom cap).
   * @return SFBool The current value of endCap.
   */
  SFBool getEndCap() const { return _endCap; }
  /**
   * @brief Data-layer write of endCap (reader/init ingest path).
   * @details endCap is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setEndCap().
   */
  void setEndCapUnchecked(const SFBool &value) { _endCap = value; }

  /**
   * @brief Gets the value of orientation. AccessType: initializeOnly
   * @details The orientation array is a list of axis-angle 4-tuple values
   * applied at each spine-aligned cross-section plane.
   * @return MFRotation The current value of orientation.
   */
  MFRotation getOrientation() const { return _orientation; }
  /**
   * @brief Data-layer write of orientation (reader/init ingest path).
   * @details orientation is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setOrientation().
   */
  void setOrientationUnchecked(const MFRotation &value) {
    _orientation = value;
  }

  /**
   * @brief Gets the value of scale. AccessType: initializeOnly
   * @details scale is a list of 2D-scale parameters applied at each
   * spine-aligned cross-section plane.
   * @return MFVec2f The current value of scale.
   */
  MFVec2f getScale() const { return _scale; }
  /**
   * @brief Data-layer write of scale (reader/init ingest path).
   * @details scale is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setScale().
   */
  void setScaleUnchecked(const MFVec2f &value) { _scale = value; }

  /**
   * @brief Event handler invoked when an event is received on set_crossSection.
   * AccessType: inputOnly
   * @details The crossSection array defines a silhouette outline of the outer
   * Extrusion surface. Dispatches to the handler registered via
   * setOnSet_crossSectionHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_crossSection(const MFVec2f &value) {
    if (_onSet_crossSectionHandler)
      _onSet_crossSectionHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_crossSection.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_crossSectionHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_crossSectionHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_orientation.
   * AccessType: inputOnly
   * @details The orientation array is a list of axis-angle 4-tuple values
   * applied at each spine-aligned cross-section plane. Dispatches to the
   * handler registered via setOnSet_orientationHandler(); a no-op if none is
   * set. The event cascade reaches this through the node's reflected field
   * table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_orientation(const MFRotation &value) {
    if (_onSet_orientationHandler)
      _onSet_orientationHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_orientation.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_orientationHandler(std::function<void(const MFRotation &)> handler) {
    _onSet_orientationHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_scale.
   * AccessType: inputOnly
   * @details scale is a list of 2D-scale parameters applied at each
   * spine-aligned cross-section plane. Dispatches to the handler registered via
   * setOnSet_scaleHandler(); a no-op if none is set. The event cascade reaches
   * this through the node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_scale(const MFVec2f &value) {
    if (_onSet_scaleHandler)
      _onSet_scaleHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_scale.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_scaleHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_scaleHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_spine.
   * AccessType: inputOnly
   * @details The spine array defines a center-line sequence of 3D points that
   * define a piecewise-linear curve forming a series of connected vertices.
   *          Dispatches to the handler registered via setOnSet_spineHandler();
   *          a no-op if none is set. The event cascade reaches this through the
   *          node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_spine(const MFVec3f &value) {
    if (_onSet_spineHandler)
      _onSet_spineHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_spine.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_spineHandler(std::function<void(const MFVec3f &)> handler) {
    _onSet_spineHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of solid. AccessType: initializeOnly
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   * @return SFBool The current value of solid.
   */
  SFBool getSolid() const { return _solid; }
  /**
   * @brief Data-layer write of solid (reader/init ingest path).
   * @details solid is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSolid().
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * @brief Gets the value of spine. AccessType: initializeOnly
   * @details The spine array defines a center-line sequence of 3D points that
   * define a piecewise-linear curve forming a series of connected vertices.
   * @return MFVec3f The current value of spine.
   */
  MFVec3f getSpine() const { return _spine; }
  /**
   * @brief Data-layer write of spine (reader/init ingest path).
   * @details spine is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSpine().
   */
  void setSpineUnchecked(const MFVec3f &value) { _spine = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Extrusion").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
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
  /**
   * @brief Member variable for beginCap.
   */

  SFBool _beginCap{true};

  /**
   * @brief Member variable for ccw.
   */

  SFBool _ccw{true};

  /**
   * @brief Member variable for convex.
   */

  SFBool _convex{true};

  /**
   * @brief Member variable for creaseAngle.
   */

  SFFloat _creaseAngle{0};

  /**
   * @brief Member variable for crossSection.
   */

  MFVec2f _crossSection{std::vector<SFVec2f>{SFVec2f{1, 1}, SFVec2f{1, -1},
                                             SFVec2f{-1, -1}, SFVec2f{-1, 1},
                                             SFVec2f{1, 1}}};

  /**
   * @brief Member variable for endCap.
   */

  SFBool _endCap{true};

  /**
   * @brief Member variable for orientation.
   */

  MFRotation _orientation{std::vector<SFRotation>{SFRotation{0, 0, 1, 0}}};

  /**
   * @brief Member variable for scale.
   */

  MFVec2f _scale{std::vector<SFVec2f>{SFVec2f{1, 1}}};

  /**
   * @brief Registered event handler for set_crossSection (inputOnly); empty
   * until set.
   */
  std::function<void(const MFVec2f &)> _onSet_crossSectionHandler{};

  /**
   * @brief Registered event handler for set_orientation (inputOnly); empty
   * until set.
   */
  std::function<void(const MFRotation &)> _onSet_orientationHandler{};

  /**
   * @brief Registered event handler for set_scale (inputOnly); empty until set.
   */
  std::function<void(const MFVec2f &)> _onSet_scaleHandler{};

  /**
   * @brief Registered event handler for set_spine (inputOnly); empty until set.
   */
  std::function<void(const MFVec3f &)> _onSet_spineHandler{};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for spine.
   */

  MFVec3f _spine{std::vector<SFVec3f>{SFVec3f{0, 0, 0}, SFVec3f{0, 1, 0}}};
};

} // namespace x3d::nodes
