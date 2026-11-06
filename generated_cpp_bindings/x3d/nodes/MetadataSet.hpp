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
 * @class MetadataSet
 * @brief The metadata provided by this node is contained in the metadata nodes
 * of the value field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#MetadataSet
 */
class MetadataSet : public virtual X3DNode, public virtual X3DMetadataObject {
public:
  MetadataSet() = default;

  ~MetadataSet() = default;

  static SFNode getDefaultIS() { return nullptr; }

  static SFNode getDefaultMetadata() { return nullptr; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesMetadata";
  }

  static std::string getDefaultContainerField() { return "value"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getIS() const { return _IS; }

  /**
   * @details Permitted X3D node types: IS
   */
  static const std::vector<std::string> &acceptableISNodeTypes() {
    static const std::vector<std::string> types = {"IS"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   */
  const SFNode &getMetadata() const { return _metadata; }

  /**
   * @details Permitted X3D node types: X3DMetadataObject
   */
  static const std::vector<std::string> &acceptableMetadataNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The value field provides a list of X3DMetadataObject nodes whose
   * meaning is determined by the name field.
   */
  const MFNode &getValue() const { return _value; }

  /**
   * @details Permitted X3D node types: X3DMetadataObject
   */
  static const std::vector<std::string> &acceptableValueNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The value field provides a list of X3DMetadataObject nodes whose
   * meaning is determined by the name field.
   */
  void setValue(const MFNode &value) { _value = value; }

  void setValue(MFNode &&value) { _value = std::move(value); }

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
  SFNode _IS{nullptr};

  SFNode _metadata{nullptr};

  MFNode _value{};
};

} // namespace x3d::nodes
