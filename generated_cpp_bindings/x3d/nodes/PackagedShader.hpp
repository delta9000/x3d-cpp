#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DShaderNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

#include "x3d/nodes/X3DProgrammableShaderObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PackagedShader
 * @brief PackagedShader can contain field declarations, but no CDATA section of
 * plain-text source code.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#PackagedShader
 */
class PackagedShader : public virtual X3DShaderNode,
                       public virtual X3DUrlObject,
                       public virtual X3DProgrammableShaderObject {
public:
  PackagedShader() = default;

  ~PackagedShader() = default;

  static SFNode getDefaultIS() { return nullptr; }

  static SFNode getDefaultMetadata() { return nullptr; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesPackagedShader";
  }

  static std::string getDefaultContainerField() { return "shaders"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Include a field statement for each field declaration in the
   * PackagedShader node.
   */
  const MFNode &getField() const { return _field; }

  /**
   * @details Permitted X3D node types: field
   */
  static const std::vector<std::string> &acceptableFieldNodeTypes() {
    static const std::vector<std::string> types = {"field"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Include a field statement for each field declaration in the
   * PackagedShader node.
   */
  void setField(const MFNode &value) { _field = value; }

  void setField(MFNode &&value) { _field = std::move(value); }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  MFNode _field{};

  SFNode _IS{nullptr};

  SFNode _metadata{nullptr};
};

} // namespace x3d::nodes
