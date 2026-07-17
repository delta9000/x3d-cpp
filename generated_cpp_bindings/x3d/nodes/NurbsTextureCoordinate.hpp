// NurbsTextureCoordinate.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsTextureCoordinate
 * @brief NurbsTextureCoordinate describes a 3D NURBS surface in the parametric
 * domain of its surface host, specifying mapping of texture onto the surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsTextureCoordinate
 */
class NurbsTextureCoordinate : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for NurbsTextureCoordinate
   */
  NurbsTextureCoordinate() = default;

  /**
   * @brief Destructor for NurbsTextureCoordinate
   */
  ~NurbsTextureCoordinate() = default;

  /**
   * @brief Get the default value for uDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUDimension() { return 0; }

  /**
   * @brief Get the default value for uOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUOrder() { return 3; }

  /**
   * @brief Get the default value for vDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVDimension() { return 0; }

  /**
   * @brief Get the default value for vOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVOrder() { return 3; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "texCoord"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of controlPoint. AccessType: inputOutput
   * @details controlPoint defines a set of control points of dimension
   * uDimension by vDimension, and defines a mesh where the points do not have
   * uniform spacing.
   * @return MFVec2f The current value of controlPoint.
   */
  MFVec2f getControlPoint() const { return _controlPoint; }

  /**
   * @brief Sets the value of controlPoint. AccessType: inputOutput
   * @details controlPoint defines a set of control points of dimension
   * uDimension by vDimension, and defines a mesh where the points do not have
   * uniform spacing.
   * @param value The new value for controlPoint.
   */
  void setControlPoint(const MFVec2f &value) { _controlPoint = value; }

  void setControlPoint(MFVec2f &&value) { _controlPoint = std::move(value); }

  /**
   * @brief Gets the value of uDimension. AccessType: initializeOnly
   * @details Number of control points in u dimension.
   * @return SFInt32 The current value of uDimension.
   */
  SFInt32 getUDimension() const { return _uDimension; }
  /**
   * @brief Data-layer write of uDimension (reader/init ingest path).
   * @details uDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUDimension().
   */
  void setUDimensionUnchecked(const SFInt32 &value) { _uDimension = value; }

  /**
   * @brief Gets the value of uKnot. AccessType: initializeOnly
   * @details Knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of uKnot.
   */
  MFDouble getUKnot() const { return _uKnot; }
  /**
   * @brief Data-layer write of uKnot (reader/init ingest path).
   * @details uKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUKnot().
   */
  void setUKnotUnchecked(const MFDouble &value) { _uKnot = value; }

  /**
   * @brief Gets the value of uOrder. AccessType: initializeOnly
   * @details Define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of uOrder.
   */
  SFInt32 getUOrder() const { return _uOrder; }
  /**
   * @brief Data-layer write of uOrder (reader/init ingest path).
   * @details uOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUOrder().
   */
  void setUOrderUnchecked(const SFInt32 &value) { _uOrder = value; }

  /**
   * @brief Gets the value of vDimension. AccessType: initializeOnly
   * @details Number of control points in v dimension.
   * @return SFInt32 The current value of vDimension.
   */
  SFInt32 getVDimension() const { return _vDimension; }
  /**
   * @brief Data-layer write of vDimension (reader/init ingest path).
   * @details vDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVDimension().
   */
  void setVDimensionUnchecked(const SFInt32 &value) { _vDimension = value; }

  /**
   * @brief Gets the value of vKnot. AccessType: initializeOnly
   * @details Knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of vKnot.
   */
  MFDouble getVKnot() const { return _vKnot; }
  /**
   * @brief Data-layer write of vKnot (reader/init ingest path).
   * @details vKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVKnot().
   */
  void setVKnotUnchecked(const MFDouble &value) { _vKnot = value; }

  /**
   * @brief Gets the value of vOrder. AccessType: initializeOnly
   * @details Define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of vOrder.
   */
  SFInt32 getVOrder() const { return _vOrder; }
  /**
   * @brief Data-layer write of vOrder (reader/init ingest path).
   * @details vOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVOrder().
   */
  void setVOrderUnchecked(const SFInt32 &value) { _vOrder = value; }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to
   * knots.
   * @return MFDouble The current value of weight.
   */
  MFDouble getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to
   * knots.
   * @param value The new value for weight.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsTextureCoordinate").
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
  static void checkRangesUDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesUOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for controlPoint.
   */

  MFVec2f _controlPoint{};

  /**
   * @brief Member variable for uDimension.
   */

  SFInt32 _uDimension{0};

  /**
   * @brief Member variable for uKnot.
   */

  MFDouble _uKnot{};

  /**
   * @brief Member variable for uOrder.
   */

  SFInt32 _uOrder{3};

  /**
   * @brief Member variable for vDimension.
   */

  SFInt32 _vDimension{0};

  /**
   * @brief Member variable for vKnot.
   */

  MFDouble _vKnot{};

  /**
   * @brief Member variable for vOrder.
   */

  SFInt32 _vOrder{3};

  /**
   * @brief Member variable for weight.
   */

  MFDouble _weight{};
};

} // namespace x3d::nodes
