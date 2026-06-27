// MetadataInteger.hpp
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
 * @class MetadataInteger
 * @brief The metadata provided by this node is contained in the integer numbers
 * of the value field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#MetadataInteger
 */
class MetadataInteger : public virtual X3DNode,
                        public virtual X3DMetadataObject {
public:
  /**
   * @brief Default constructor for MetadataInteger
   */
  MetadataInteger() = default;

  /**
   * @brief Destructor for MetadataInteger
   */
  ~MetadataInteger() = default;

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
   * @return MFInt32 The current value of value.
   */
  MFInt32 getValue() const { return _value; }

  /**
   * @brief Sets the value of value. AccessType: inputOutput
   * @details The value attribute is a strictly typed data array providing
   * relevant metadata information.
   * @param value The new value for value.
   */
  void setValue(const MFInt32 &value) { _value = value; }

  void setValue(MFInt32 &&value) { _value = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MetadataInteger").
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

  MFInt32 _value{};
};

} // namespace x3d::nodes
