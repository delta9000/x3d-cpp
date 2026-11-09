#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ShaderPart
 * @brief ShaderPart can contain a CDATA section of plain-text source code.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#ShaderPart
 */
class ShaderPart : public virtual X3DNode, public virtual X3DUrlObject {
public:
  ShaderPart() = default;

  ~ShaderPart() = default;

  static SFString getDefaultType() { return "VERTEX"; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesShaderPart";
  }

  static std::string getDefaultContainerField() { return "parts"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Embedded scripting source code can be contained within the parent
   * node as a plain-text CDATA block, without requiring escaping of special
   * characters.
   */
  const SFString &getSourceCode() const { return _sourceCode; }

  /**
   * Access: inputOutput.
   * @details Embedded scripting source code can be contained within the parent
   * node as a plain-text CDATA block, without requiring escaping of special
   * characters.
   */
  void setSourceCode(const SFString &value) { _sourceCode = value; }

  void setSourceCode(SFString &&value) { _sourceCode = std::move(value); }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details type indicates whether this ShaderProgram is a vertex or fragment
   * (pixel) shader.
   */
  const SFString &getType() const { return _type; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTypeUnchecked(const SFString &value) { _type = value; }

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
  SFString _sourceCode{};

  SFString _type{"VERTEX"};
};

} // namespace x3d::nodes
