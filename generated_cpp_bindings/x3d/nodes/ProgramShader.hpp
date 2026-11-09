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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ProgramShader
 * @brief ProgramShader contains no field declarations and no plain-text source
 * code.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#ProgramShader
 */
class ProgramShader : public virtual X3DShaderNode {
public:
  ProgramShader() = default;

  ~ProgramShader() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "shaders"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details ProgramShader contains zero or more ShaderProgram node instances.
   */
  const MFNode &getPrograms() const { return _programs; }

  /**
   * @details Permitted X3D node types: ShaderProgram
   */
  static const std::vector<std::string> &acceptableProgramsNodeTypes() {
    static const std::vector<std::string> types = {"ShaderProgram"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details ProgramShader contains zero or more ShaderProgram node instances.
   */
  void setPrograms(const MFNode &value) { _programs = value; }

  void setPrograms(MFNode &&value) { _programs = std::move(value); }

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
  MFNode _programs{};
};

} // namespace x3d::nodes
