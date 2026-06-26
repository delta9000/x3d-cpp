// Polypoint2D.hpp
#ifndef POLYPOINT2D_HPP
#define POLYPOINT2D_HPP

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
 * @class Polypoint2D
 * @brief Polypoint2D is a geometry node that defines a set of 2D points in X-Y
 * plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Polypoint2D
 */
class Polypoint2D : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Polypoint2D
   */
  Polypoint2D() = default;

  /**
   * @brief Destructor for Polypoint2D
   */
  ~Polypoint2D() = default;

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
   * @brief Gets the value of point. AccessType: inputOutput
   * @details 2D coordinates of vertices.
   * @return MFVec2f The current value of point.
   */
  MFVec2f getPoint() const { return _point; }

  /**
   * @brief Sets the value of point. AccessType: inputOutput
   * @details 2D coordinates of vertices.
   * @param value The new value for point.
   */
  void setPoint(const MFVec2f &value) { _point = value; }

  void setPoint(MFVec2f &&value) { _point = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "Polypoint2D").
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
   * @brief Member variable for point.
   */

  MFVec2f _point{};
};

#endif // POLYPOINT2D_HPP