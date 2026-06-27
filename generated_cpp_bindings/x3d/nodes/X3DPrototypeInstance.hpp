// X3DPrototypeInstance.hpp
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
 * @class X3DPrototypeInstance
 * @brief Base type for all prototype instances. Note that direct children nodes
 * are disallowed, instead let fieldValue with type SFNode/MFNode contain them.
 * Current practice is that, if desired, prototype authors must explicitly add
 * the metadata SFNode field in the ProtoInterface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DPrototypeInstance
 */
class X3DPrototypeInstance : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for X3DPrototypeInstance
   */
  X3DPrototypeInstance() = default;

  /**
   * @brief Virtual destructor for X3DPrototypeInstance
   */
  virtual ~X3DPrototypeInstance() = default;

  /**
   * @brief Get the default value for IS
   * @return SFNode The default value
   */
  static SFNode getDefaultIS() { return nullptr; }

  /**
   * @brief Get the default value for metadata
   * @return SFNode The default value
   */
  static SFNode getDefaultMetadata() { return nullptr; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of IS. AccessType: inputOutput
   * @details
   * @return SFNode The current value of IS.
   */
  SFNode getIS() const { return _IS; }

  /**
   * @brief Acceptable node types for the IS field.
   * @details Permitted X3D node types: IS
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableISNodeTypes() {
    static const std::vector<std::string> types = {"IS"};
    return types;
  }

  /**
   * @brief Sets the value of IS. AccessType: inputOutput
   * @details
   * @param value The new value for IS.
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * @brief Gets the value of metadata. AccessType: inputOutput
   * @details
   * @return SFNode The current value of metadata.
   */
  SFNode getMetadata() const { return _metadata; }

  /**
   * @brief Acceptable node types for the metadata field.
   * @details Permitted X3D node types: X3DMetadataObject
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMetadataNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * @brief Sets the value of metadata. AccessType: inputOutput
   * @details
   * @param value The new value for metadata.
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DPrototypeInstance").
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
   * @brief Member variable for IS.
   */

  SFNode _IS{nullptr};

  /**
   * @brief Member variable for metadata.
   */

  SFNode _metadata{nullptr};
};

} // namespace x3d::nodes
