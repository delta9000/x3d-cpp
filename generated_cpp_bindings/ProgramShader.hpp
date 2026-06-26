// ProgramShader.hpp
#ifndef PROGRAMSHADER_HPP
#define PROGRAMSHADER_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DAppearanceChildNode.hpp"

#include "X3DShaderNode.hpp"

/**
 * @class ProgramShader
 * @brief ProgramShader contains no field declarations and no plain-text source
 * code.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#ProgramShader
 */
class ProgramShader : public virtual X3DShaderNode {
public:
  /**
   * @brief Default constructor for ProgramShader
   */
  ProgramShader() = default;

  /**
   * @brief Destructor for ProgramShader
   */
  ~ProgramShader() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "shaders"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shaders"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of programs. AccessType: inputOutput
   * @details ProgramShader contains zero or more ShaderProgram node instances.
   * @return MFNode The current value of programs.
   */
  MFNode getPrograms() const { return _programs; }

  /**
   * @brief Acceptable node types for the programs field.
   * @details Permitted X3D node types: ShaderProgram
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableProgramsNodeTypes() {
    static const std::vector<std::string> types = {"ShaderProgram"};
    return types;
  }

  /**
   * @brief Sets the value of programs. AccessType: inputOutput
   * @details ProgramShader contains zero or more ShaderProgram node instances.
   * @param value The new value for programs.
   */
  void setPrograms(const MFNode &value) { _programs = value; }

  void setPrograms(MFNode &&value) { _programs = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ProgramShader").
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
   * @brief Member variable for programs.
   */

  MFNode _programs{};
};

#endif // PROGRAMSHADER_HPP