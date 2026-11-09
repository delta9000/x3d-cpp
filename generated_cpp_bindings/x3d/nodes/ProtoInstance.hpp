#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DPrototypeInstance.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

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
  ProtoInstance() = default;

  ~ProtoInstance() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Include fieldValue statements if this ProtoInstance overrides
   * default values in any of the original field declarations.
   */
  const MFNode &getFieldValue() const { return _fieldValue; }

  /**
   * @details Permitted X3D node types: fieldValue
   */
  static const std::vector<std::string> &acceptableFieldValueNodeTypes() {
    static const std::vector<std::string> types = {"fieldValue"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Include fieldValue statements if this ProtoInstance overrides
   * default values in any of the original field declarations.
   */
  void setFieldValue(const MFNode &value) { _fieldValue = value; }

  void setFieldValue(MFNode &&value) { _fieldValue = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details name of the prototype node being instanced.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details name of the prototype node being instanced.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  MFNode _fieldValue{};

  SFString _name{};
};

} // namespace x3d::nodes
