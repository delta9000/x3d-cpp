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
 * @class MetadataString
 * @brief The metadata provided by this node is contained in the strings of the
 * value field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#MetadataString
 */
class MetadataString : public virtual X3DNode,
                       public virtual X3DMetadataObject {
public:
  MetadataString() = default;

  ~MetadataString() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesMetadata";
  }

  static std::string getDefaultContainerField() { return "value"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details The value attribute is a strictly typed data array providing
   * relevant metadata information.
   */
  const MFString &getValue() const { return _value; }

  /**
   * Access: inputOutput.
   * @details The value attribute is a strictly typed data array providing
   * relevant metadata information.
   */
  void setValue(const MFString &value) { _value = value; }

  void setValue(MFString &&value) { _value = std::move(value); }

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
  MFString _value{};
};

} // namespace x3d::nodes
