// Disk2D.hpp
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
  /**
   * @brief Default constructor for Disk2D
   */
  Disk2D() = default;

  /**
   * @brief Destructor for Disk2D
   */
  ~Disk2D() = default;

  /**
   * @brief Get the default value for innerRadius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultInnerRadius() { return 0; }

  /**
   * @brief Get the default value for outerRadius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultOuterRadius() { return 1; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return false; }

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
   * @brief Gets the value of innerRadius. AccessType: initializeOnly
   * @details Inner circle radius, greater than or equal to 0.
   * @return SFFloat The current value of innerRadius.
   */
  SFFloat getInnerRadius() const { return _innerRadius; }
  /**
   * @brief Data-layer write of innerRadius (reader/init ingest path).
   * @details innerRadius is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setInnerRadius().
   */
  void setInnerRadiusUnchecked(const SFFloat &value) { _innerRadius = value; }

  /**
   * @brief Gets the value of outerRadius. AccessType: initializeOnly
   * @details Outer radius of circle, greater than or equal to inner radius.
   * @return SFFloat The current value of outerRadius.
   */
  SFFloat getOuterRadius() const { return _outerRadius; }
  /**
   * @brief Data-layer write of outerRadius (reader/init ingest path).
   * @details outerRadius is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setOuterRadius().
   */
  void setOuterRadiusUnchecked(const SFFloat &value) { _outerRadius = value; }

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
   * @brief The X3D type name of this node (e.g. "Disk2D").
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
  static void checkRangesInnerRadius(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for innerRadius.
   */

  SFFloat _innerRadius{0};

  /**
   * @brief Member variable for outerRadius.
   */

  SFFloat _outerRadius{1};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{false};
};

} // namespace x3d::nodes
