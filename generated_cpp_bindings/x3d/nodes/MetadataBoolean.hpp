// MetadataBoolean.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DMetadataObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class MetadataBoolean
 * @brief The metadata provided by this node is contained in the Boolean values
 * of the value field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#MetadataBoolean
 */
class MetadataBoolean : public virtual X3DNode,
                        public virtual X3DMetadataObject {
public:
  /**
   * @brief Default constructor for MetadataBoolean
   */
  MetadataBoolean() = default;

  /**
   * @brief Destructor for MetadataBoolean
   */
  ~MetadataBoolean() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesMetadata";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "value"; }

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
   * @brief Gets the value of value. AccessType: inputOutput
   * @details The value attribute is a strictly typed data array providing
   * relevant metadata information.
   * @return MFBool The current value of value.
   */
  MFBool getValue() const { return _value; }

  /**
   * @brief Sets the value of value. AccessType: inputOutput
   * @details The value attribute is a strictly typed data array providing
   * relevant metadata information.
   * @param value The new value for value.
   */
  void setValue(const MFBool &value) { _value = value; }

  void setValue(MFBool &&value) { _value = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MetadataBoolean").
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
   * @brief Member variable for value.
   */

  MFBool _value{};
};

} // namespace x3d::nodes
