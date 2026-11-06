#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

#include "x3d/nodes/X3DScriptNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Script
 * @brief Script contains author-programmed event behaviors for a scene.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/scripting.html#Script
 */
class Script : public virtual X3DScriptNode {
public:
  Script() = default;

  ~Script() = default;

  static SFBool getDefaultDirectOutput() { return false; }

  static SFBool getDefaultMustEvaluate() { return false; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Scripting"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Set directOutput true if Script has field reference(s) of type
   * SFNode/MFNode, and also uses direct access to modify attributes of a
   * referenced node in the Scene.
   */
  const SFBool &getDirectOutput() const { return _directOutput; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setDirectOutputUnchecked(const SFBool &value) { _directOutput = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Include a field statement for each field declaration in this
   * Script node.
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
   * @details Include a field statement for each field declaration in this
   * Script node.
   */
  void setField(const MFNode &value) { _field = value; }

  void setField(MFNode &&value) { _field = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details If mustEvaluate false, then the X3D player may delay sending input
   * events to Script until output events are needed.
   */
  const SFBool &getMustEvaluate() const { return _mustEvaluate; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setMustEvaluateUnchecked(const SFBool &value) { _mustEvaluate = value; }

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
  SFBool _directOutput{false};

  MFNode _field{};

  SFBool _mustEvaluate{false};

  SFString _sourceCode{};
};

} // namespace x3d::nodes
