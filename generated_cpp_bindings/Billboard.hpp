// Billboard.hpp
#ifndef BILLBOARD_HPP
#define BILLBOARD_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

/**
 * @class Billboard
 * @brief Billboard is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#Billboard
 */
class Billboard : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for Billboard
   */
  Billboard() = default;

  /**
   * @brief Destructor for Billboard
   */
  ~Billboard() = default;

  /**
   * @brief Get the default value for axisOfRotation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAxisOfRotation() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of axisOfRotation. AccessType: inputOutput
   * @details axisOfRotation direction is relative to local coordinate system.
   * @return SFVec3f The current value of axisOfRotation.
   */
  SFVec3f getAxisOfRotation() const { return _axisOfRotation; }

  /**
   * @brief Sets the value of axisOfRotation. AccessType: inputOutput
   * @details axisOfRotation direction is relative to local coordinate system.
   * @param value The new value for axisOfRotation.
   */
  void setAxisOfRotation(const SFVec3f &value) { _axisOfRotation = value; }

  void setAxisOfRotation(SFVec3f &&value) {

    _axisOfRotation = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "Billboard").
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
   * @brief Member variable for axisOfRotation.
   */

  SFVec3f _axisOfRotation{SFVec3f{0, 1, 0}};
};

#endif // BILLBOARD_HPP