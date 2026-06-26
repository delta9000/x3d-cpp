// ProtoInstance.hpp
#ifndef PROTOINSTANCE_HPP
#define PROTOINSTANCE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DPrototypeInstance.hpp"

#include "X3DChildNode.hpp"

/**
 * @class ProtoInstance
 * @brief ProtoInstance can override field default values via fieldValue
 * initializations. Non-recursive nested ProtoInstance and ProtoDeclare
 * statements are allowed within a ProtoDeclare.
 * @details
 * https://www.web3d.org/documents/specifications/19776-1/V3.3/Part01/concepts.html#ProtoInstanceAndFieldValueStatement
 */
class ProtoInstance : public virtual X3DPrototypeInstance,
                      public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for ProtoInstance
   */
  ProtoInstance() = default;

  /**
   * @brief Destructor for ProtoInstance
   */
  ~ProtoInstance() = default;

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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of fieldValue. AccessType: inputOutput
   * @details Include fieldValue statements if this ProtoInstance overrides
   * default values in any of the original field declarations.
   * @return MFNode The current value of fieldValue.
   */
  MFNode getFieldValue() const { return _fieldValue; }

  /**
   * @brief Acceptable node types for the fieldValue field.
   * @details Permitted X3D node types: fieldValue
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFieldValueNodeTypes() {
    static const std::vector<std::string> types = {"fieldValue"};
    return types;
  }

  /**
   * @brief Sets the value of fieldValue. AccessType: inputOutput
   * @details Include fieldValue statements if this ProtoInstance overrides
   * default values in any of the original field declarations.
   * @param value The new value for fieldValue.
   */
  void setFieldValue(const MFNode &value) { _fieldValue = value; }

  void setFieldValue(MFNode &&value) { _fieldValue = std::move(value); }

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details name of the prototype node being instanced.
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details name of the prototype node being instanced.
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ProtoInstance").
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
   * @brief Member variable for fieldValue.
   */

  MFNode _fieldValue{};

  /**
   * @brief Member variable for name.
   */

  SFString _name{};
};

#endif // PROTOINSTANCE_HPP