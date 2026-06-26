// Cylinder.hpp
#ifndef CYLINDER_HPP
#define CYLINDER_HPP

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
 * @class Cylinder
 * @brief Cylinder is a geometry node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#Cylinder
 */
class Cylinder : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Cylinder
   */
  Cylinder() = default;

  /**
   * @brief Destructor for Cylinder
   */
  ~Cylinder() = default;

  /**
   * @brief Get the default value for bottom
   * @return SFBool The default value
   */
  static SFBool getDefaultBottom() { return true; }

  /**
   * @brief Get the default value for height
   * @return SFFloat The default value
   */
  static SFFloat getDefaultHeight() { return 2; }

  /**
   * @brief Get the default value for radius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRadius() { return 1; }

  /**
   * @brief Get the default value for side
   * @return SFBool The default value
   */
  static SFBool getDefaultSide() { return true; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

  /**
   * @brief Get the default value for top
   * @return SFBool The default value
   */
  static SFBool getDefaultTop() { return true; }

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of bottom. AccessType: inputOutput
   * @details Whether to draw bottom (inside faces are never drawn).
   * @return SFBool The current value of bottom.
   */
  SFBool getBottom() const { return _bottom; }

  /**
   * @brief Sets the value of bottom. AccessType: inputOutput
   * @details Whether to draw bottom (inside faces are never drawn).
   * @param value The new value for bottom.
   */
  void setBottom(const SFBool &value) { _bottom = value; }

  /**
   * @brief Gets the value of height. AccessType: initializeOnly
   * @details Size in meters.
   * @return SFFloat The current value of height.
   */
  SFFloat getHeight() const { return _height; }
  /**
   * @brief Data-layer write of height (reader/init ingest path).
   * @details height is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setHeight().
   */
  void setHeightUnchecked(const SFFloat &value) { _height = value; }

  /**
   * @brief Gets the value of radius. AccessType: initializeOnly
   * @details Size in meters.
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
   * @brief Gets the value of side. AccessType: inputOutput
   * @details Whether to draw sides (inside faces are never drawn).
   * @return SFBool The current value of side.
   */
  SFBool getSide() const { return _side; }

  /**
   * @brief Sets the value of side. AccessType: inputOutput
   * @details Whether to draw sides (inside faces are never drawn).
   * @param value The new value for side.
   */
  void setSide(const SFBool &value) { _side = value; }

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
   * @brief Gets the value of top. AccessType: inputOutput
   * @details Whether to draw top (inside faces are never drawn).
   * @return SFBool The current value of top.
   */
  SFBool getTop() const { return _top; }

  /**
   * @brief Sets the value of top. AccessType: inputOutput
   * @details Whether to draw top (inside faces are never drawn).
   * @param value The new value for top.
   */
  void setTop(const SFBool &value) { _top = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Cylinder").
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
   * @brief Member variable for bottom.
   */

  SFBool _bottom{true};

  /**
   * @brief Member variable for height.
   */

  SFFloat _height{2};

  /**
   * @brief Member variable for radius.
   */

  SFFloat _radius{1};

  /**
   * @brief Member variable for side.
   */

  SFBool _side{true};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for top.
   */

  SFBool _top{true};
};

#endif // CYLINDER_HPP