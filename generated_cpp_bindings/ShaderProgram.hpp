// ShaderProgram.hpp
#ifndef SHADERPROGRAM_HPP
#define SHADERPROGRAM_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DUrlObject.hpp"

#include "X3DProgrammableShaderObject.hpp"

/**
 * @class ShaderProgram
 * @brief ShaderProgram can contain field declarations and a CDATA section of
 * plain-text source code.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#ShaderProgram
 */
class ShaderProgram : public virtual X3DNode,
                      public virtual X3DUrlObject,
                      public virtual X3DProgrammableShaderObject {
public:
  /**
   * @brief Default constructor for ShaderProgram
   */
  ShaderProgram() = default;

  /**
   * @brief Destructor for ShaderProgram
   */
  ~ShaderProgram() = default;

  /**
   * @brief Get the default value for type
   * @return SFString The default value
   */
  static SFString getDefaultType() { return "VERTEX"; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "programs"; }

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
   * @brief Gets the value of field. AccessType: inputOutput
   * @details Include a field statement for each field declaration in the
   * ShaderProgram node.
   * @return MFNode The current value of field.
   */
  MFNode getField() const { return _field; }

  /**
   * @brief Acceptable node types for the field field.
   * @details Permitted X3D node types: field
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFieldNodeTypes() {
    static const std::vector<std::string> types = {"field"};
    return types;
  }

  /**
   * @brief Sets the value of field. AccessType: inputOutput
   * @details Include a field statement for each field declaration in the
   * ShaderProgram node.
   * @param value The new value for field.
   */
  void setField(const MFNode &value) { _field = value; }

  void setField(MFNode &&value) { _field = std::move(value); }

  /**
   * @brief Gets the value of sourceCode. AccessType: inputOutput
   * @details Embedded scripting source code can be contained within the parent
   * node as a plain-text CDATA block, without requiring escaping of special
   * characters.
   * @return SFString The current value of sourceCode.
   */
  SFString getSourceCode() const { return _sourceCode; }

  /**
   * @brief Sets the value of sourceCode. AccessType: inputOutput
   * @details Embedded scripting source code can be contained within the parent
   * node as a plain-text CDATA block, without requiring escaping of special
   * characters.
   * @param value The new value for sourceCode.
   */
  void setSourceCode(const SFString &value) { _sourceCode = value; }

  void setSourceCode(SFString &&value) { _sourceCode = std::move(value); }

  /**
   * @brief Gets the value of type. AccessType: initializeOnly
   * @details type indicates whether this ShaderProgram is a vertex or fragment
   * (pixel) shader.
   * @return SFString The current value of type.
   */
  SFString getType() const { return _type; }
  /**
   * @brief Data-layer write of type (reader/init ingest path).
   * @details type is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setType().
   */
  void setTypeUnchecked(const SFString &value) { _type = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ShaderProgram").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  /**
   * @brief Member variable for field.
   */

  MFNode _field{};

  /**
   * @brief Member variable for sourceCode.
   */

  SFString _sourceCode{};

  /**
   * @brief Member variable for type.
   */

  SFString _type{"VERTEX"};
};

#endif // SHADERPROGRAM_HPP