// Box.hpp
#ifndef BOX_HPP
#define BOX_HPP

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
 * @class Box
 * @brief Box is a geometry node specifying a rectangular cuboid.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#Box
 */
class Box : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Box
   */
  Box() = default;

  /**
   * @brief Destructor for Box
   */
  ~Box() = default;

  /**
   * @brief Get the default value for size
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultSize() { return SFVec3f{2, 2, 2}; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

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
   * @brief Gets the value of size. AccessType: initializeOnly
   * @details size x y z in meters.
   * @return SFVec3f The current value of size.
   */
  SFVec3f getSize() const { return _size; }
  /**
   * @brief Data-layer write of size (reader/init ingest path).
   * @details size is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSize().
   */
  void setSizeUnchecked(const SFVec3f &value) { _size = value; }

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
   * @brief The X3D type name of this node (e.g. "Box").
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
   * @brief Member variable for size.
   */

  SFVec3f _size{SFVec3f{2, 2, 2}};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};
};

#endif // BOX_HPP