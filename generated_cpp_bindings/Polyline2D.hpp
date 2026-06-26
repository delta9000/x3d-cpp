// Polyline2D.hpp
#ifndef POLYLINE2D_HPP
#define POLYLINE2D_HPP

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
 * @class Polyline2D
 * @brief Polyline2D is a geometry node that defines a connected set of vertices
 * in a contiguous set of line segments in X-Y plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Polyline2D
 */
class Polyline2D : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Polyline2D
   */
  Polyline2D() = default;

  /**
   * @brief Destructor for Polyline2D
   */
  ~Polyline2D() = default;

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of lineSegments. AccessType: initializeOnly
   * @details Coordinates of vertices connected into contiguous Polyline2D.
   * @return MFVec2f The current value of lineSegments.
   */
  MFVec2f getLineSegments() const { return _lineSegments; }
  /**
   * @brief Data-layer write of lineSegments (reader/init ingest path).
   * @details lineSegments is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setLineSegments().
   */
  void setLineSegmentsUnchecked(const MFVec2f &value) { _lineSegments = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Polyline2D").
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
   * @brief Member variable for lineSegments.
   */

  MFVec2f _lineSegments{};
};

#endif // POLYLINE2D_HPP