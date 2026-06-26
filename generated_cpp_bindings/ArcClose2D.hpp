// ArcClose2D.hpp
#ifndef ARCCLOSE2D_HPP
#define ARCCLOSE2D_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

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
  /**
   * @brief Default constructor for ArcClose2D
   */
  ArcClose2D() = default;

  /**
   * @brief Destructor for ArcClose2D
   */
  ~ArcClose2D() = default;

  /**
   * @brief Get the default value for closureType
   * @return ClosureTypeChoices The default value
   */
  static ClosureTypeChoices getDefaultClosureType() {
    return ClosureTypeChoices::PIE;
  }

  /**
   * @brief Get the default value for endAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultEndAngle() { return 1.570796; }

  /**
   * @brief Get the default value for radius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRadius() { return 1; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return false; }

  /**
   * @brief Get the default value for startAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStartAngle() { return 0; }

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
  static std::string componentName() { return "Geometry2D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of closureType. AccessType: initializeOnly
   * @details Defines whether pair of line segments connect to center (PIE), or
   * single line-segment chord connects arc endpoints (CHORD).
   * @return ClosureTypeChoices The current value of closureType.
   */
  ClosureTypeChoices getClosureType() const { return _closureType; }
  /**
   * @brief Data-layer write of closureType (reader/init ingest path).
   * @details closureType is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setClosureType().
   */
  void setClosureTypeUnchecked(const ClosureTypeChoices &value) {
    _closureType = value;
  }

  /**
   * @brief Gets the value of endAngle. AccessType: initializeOnly
   * @details Arc extends from startAngle counterclockwise to endAngle, in
   * radians.
   * @return SFFloat The current value of endAngle.
   */
  SFFloat getEndAngle() const { return _endAngle; }
  /**
   * @brief Data-layer write of endAngle (reader/init ingest path).
   * @details endAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setEndAngle().
   */
  void setEndAngleUnchecked(const SFFloat &value) { _endAngle = value; }

  /**
   * @brief Gets the value of radius. AccessType: initializeOnly
   * @details circle radius, of which the arc is a portion.
   * @return SFFloat The current value of radius.
   */
  SFFloat getRadius() const { return _radius; }
  /**
   * @brief Data-layer write of radius (reader/init ingest path).
   * @details radius is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRadius().
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

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
   * @brief Gets the value of startAngle. AccessType: initializeOnly
   * @details Arc extends from startAngle counterclockwise to endAngle, in
   * radians.
   * @return SFFloat The current value of startAngle.
   */
  SFFloat getStartAngle() const { return _startAngle; }
  /**
   * @brief Data-layer write of startAngle (reader/init ingest path).
   * @details startAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setStartAngle().
   */
  void setStartAngleUnchecked(const SFFloat &value) { _startAngle = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ArcClose2D").
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
   * @brief Member variable for closureType.
   */

  ClosureTypeChoices _closureType{ClosureTypeChoices::PIE};

  /**
   * @brief Member variable for endAngle.
   */

  SFFloat _endAngle{1.570796};

  /**
   * @brief Member variable for radius.
   */

  SFFloat _radius{1};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{false};

  /**
   * @brief Member variable for startAngle.
   */

  SFFloat _startAngle{0};
};

#endif // ARCCLOSE2D_HPP