// Script.hpp
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
  /**
   * @brief Default constructor for Script
   */
  Script() = default;

  /**
   * @brief Destructor for Script
   */
  ~Script() = default;

  /**
   * @brief Get the default value for directOutput
   * @return SFBool The default value
   */
  static SFBool getDefaultDirectOutput() { return false; }

  /**
   * @brief Get the default value for mustEvaluate
   * @return SFBool The default value
   */
  static SFBool getDefaultMustEvaluate() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Scripting"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of directOutput. AccessType: initializeOnly
   * @details Set directOutput true if Script has field reference(s) of type
   * SFNode/MFNode, and also uses direct access to modify attributes of a
   * referenced node in the Scene.
   * @return SFBool The current value of directOutput.
   */
  SFBool getDirectOutput() const { return _directOutput; }
  /**
   * @brief Data-layer write of directOutput (reader/init ingest path).
   * @details directOutput is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setDirectOutput().
   */
  void setDirectOutputUnchecked(const SFBool &value) { _directOutput = value; }

  /**
   * @brief Gets the value of field. AccessType: inputOutput
   * @details Include a field statement for each field declaration in this
   * Script node.
   * @return const MFNode& The current value of field.
   */
  const MFNode &getField() const { return _field; }

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
   * @details Include a field statement for each field declaration in this
   * Script node.
   * @param value The new value for field.
   */
  void setField(const MFNode &value) { _field = value; }

  void setField(MFNode &&value) { _field = std::move(value); }

  /**
   * @brief Gets the value of mustEvaluate. AccessType: initializeOnly
   * @details If mustEvaluate false, then the X3D player may delay sending input
   * events to Script until output events are needed.
   * @return SFBool The current value of mustEvaluate.
   */
  SFBool getMustEvaluate() const { return _mustEvaluate; }
  /**
   * @brief Data-layer write of mustEvaluate (reader/init ingest path).
   * @details mustEvaluate is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setMustEvaluate().
   */
  void setMustEvaluateUnchecked(const SFBool &value) { _mustEvaluate = value; }

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
   * @brief The X3D type name of this node (e.g. "Script").
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
   * @brief Member variable for directOutput.
   */

  SFBool _directOutput{false};

  /**
   * @brief Member variable for field.
   */

  MFNode _field{};

  /**
   * @brief Member variable for mustEvaluate.
   */

  SFBool _mustEvaluate{false};

  /**
   * @brief Member variable for sourceCode.
   */

  SFString _sourceCode{};
};

} // namespace x3d::nodes
