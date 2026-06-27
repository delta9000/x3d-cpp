// WorldInfo.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DInfoNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class WorldInfo
 * @brief WorldInfo contains a title and simple persistent metadata information
 * about an X3D scene. This node is strictly for documentation purposes and has
 * no effect on the visual appearance or behaviour of the world.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#WorldInfo
 */
class WorldInfo : public virtual X3DInfoNode {
public:
  /**
   * @brief Default constructor for WorldInfo
   */
  WorldInfo() = default;

  /**
   * @brief Destructor for WorldInfo
   */
  ~WorldInfo() = default;

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
  static std::string componentName() { return "Core"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of info. AccessType: inputOutput
   * @details Additional information about this model.
   * @return MFString The current value of info.
   */
  MFString getInfo() const { return _info; }

  /**
   * @brief Sets the value of info. AccessType: inputOutput
   * @details Additional information about this model.
   * @param value The new value for info.
   */
  void setInfo(const MFString &value) { _info = value; }

  void setInfo(MFString &&value) { _info = std::move(value); }

  /**
   * @brief Gets the value of title. AccessType: inputOutput
   * @details title of this world, placed in window title.
   * @return SFString The current value of title.
   */
  SFString getTitle() const { return _title; }

  /**
   * @brief Sets the value of title. AccessType: inputOutput
   * @details title of this world, placed in window title.
   * @param value The new value for title.
   */
  void setTitle(const SFString &value) { _title = value; }

  void setTitle(SFString &&value) { _title = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "WorldInfo").
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
   * @brief Member variable for info.
   */

  MFString _info{};

  /**
   * @brief Member variable for title.
   */

  SFString _title{};
};

} // namespace x3d::nodes
