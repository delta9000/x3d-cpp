// NurbsSweptSurface.hpp
#ifndef NURBSSWEPTSURFACE_HPP
#define NURBSSWEPTSURFACE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

#include "X3DParametricGeometryNode.hpp"

/**
 * @class NurbsSweptSurface
 * @brief NurbsSweptSurface uses a trajectoryCurve path to describe a
 * generalized surface that is swept by a crossSectionCurve.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSweptSurface
 */
class NurbsSweptSurface : public virtual X3DParametricGeometryNode {
public:
  /**
   * @brief Default constructor for NurbsSweptSurface
   */
  NurbsSweptSurface() = default;

  /**
   * @brief Destructor for NurbsSweptSurface
   */
  ~NurbsSweptSurface() = default;

  /**
   * @brief Get the default value for ccw
   * @return SFBool The default value
   */
  static SFBool getDefaultCcw() { return true; }

  /**
   * @brief Get the default value for crossSectionCurve
   * @return SFNode The default value
   */
  static SFNode getDefaultCrossSectionCurve() { return nullptr; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

  /**
   * @brief Get the default value for trajectoryCurve
   * @return SFNode The default value
   */
  static SFNode getDefaultTrajectoryCurve() { return nullptr; }

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
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of ccw. AccessType: initializeOnly
   * @details ccw defines clockwise/counterclockwise ordering of vertex
   * coordinates, which in turn defines front/back orientation of polygon
   * normals according to Right-Hand Rule (RHR).
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
   * @brief Gets the value of crossSectionCurve. AccessType: inputOutput
   * @details defines cross-section of the surface traced about the
   * trajectoryCurve axis.
   * @return SFNode The current value of crossSectionCurve.
   */
  SFNode getCrossSectionCurve() const { return _crossSectionCurve; }

  /**
   * @brief Acceptable node types for the crossSectionCurve field.
   * @details Permitted X3D node types: X3DNurbsControlCurveNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableCrossSectionCurveNodeTypes() {
    static const std::vector<std::string> types = {"X3DNurbsControlCurveNode"};
    return types;
  }

  /**
   * @brief Sets the value of crossSectionCurve. AccessType: inputOutput
   * @details defines cross-section of the surface traced about the
   * trajectoryCurve axis.
   * @param value The new value for crossSectionCurve.
   */
  void setCrossSectionCurve(const SFNode &value) { _crossSectionCurve = value; }

  void setCrossSectionCurve(SFNode &&value) {

    _crossSectionCurve = std::move(value);
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
   * @brief Gets the value of trajectoryCurve. AccessType: inputOutput
   * @details describes the center-line path using a NurbsCurve node, oriented
   * so that it is defined counterclockwise when looking down the −Y axis, thus
   * defining a concept of inside and outside.
   * @return SFNode The current value of trajectoryCurve.
   */
  SFNode getTrajectoryCurve() const { return _trajectoryCurve; }

  /**
   * @brief Acceptable node types for the trajectoryCurve field.
   * @details Permitted X3D node types: NurbsCurve
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTrajectoryCurveNodeTypes() {
    static const std::vector<std::string> types = {"NurbsCurve"};
    return types;
  }

  /**
   * @brief Sets the value of trajectoryCurve. AccessType: inputOutput
   * @details describes the center-line path using a NurbsCurve node, oriented
   * so that it is defined counterclockwise when looking down the −Y axis, thus
   * defining a concept of inside and outside.
   * @param value The new value for trajectoryCurve.
   */
  void setTrajectoryCurve(const SFNode &value) { _trajectoryCurve = value; }

  void setTrajectoryCurve(SFNode &&value) {

    _trajectoryCurve = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsSweptSurface").
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

private:
  /**
   * @brief Member variable for ccw.
   */

  SFBool _ccw{true};

  /**
   * @brief Member variable for crossSectionCurve.
   */

  SFNode _crossSectionCurve{nullptr};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for trajectoryCurve.
   */

  SFNode _trajectoryCurve{nullptr};
};

#endif // NURBSSWEPTSURFACE_HPP