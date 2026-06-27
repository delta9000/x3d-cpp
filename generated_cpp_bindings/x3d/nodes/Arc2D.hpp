// Arc2D.hpp
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
 * @class Arc2D
 * @brief Arc2D is a line-based geometry node that defines a linear circular arc
 * with center (0,0) in X-Y plane, with angles measured starting at positive
 * x-axis and sweeping towards positive y-axis.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Arc2D
 */
class Arc2D : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Arc2D
   */
  Arc2D() = default;

  /**
   * @brief Destructor for Arc2D
   */
  ~Arc2D() = default;

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
   * @brief The X3D type name of this node (e.g. "Arc2D").
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
   * @brief Member variable for endAngle.
   */

  SFFloat _endAngle{1.570796};

  /**
   * @brief Member variable for radius.
   */

  SFFloat _radius{1};

  /**
   * @brief Member variable for startAngle.
   */

  SFFloat _startAngle{0};
};

} // namespace x3d::nodes
