// FloatVertexAttribute.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

#include "x3d/nodes/X3DVertexAttributeNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class FloatVertexAttribute
 * @brief FloatVertexAttribute defines a set of per-vertex single-precision
 * floating-point attributes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#FloatVertexAttribute
 */
class FloatVertexAttribute : public virtual X3DVertexAttributeNode {
public:
  /**
   * @brief Default constructor for FloatVertexAttribute
   */
  FloatVertexAttribute() = default;

  /**
   * @brief Destructor for FloatVertexAttribute
   */
  ~FloatVertexAttribute() = default;

  /**
   * @brief Get the default value for numComponents
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultNumComponents() { return 4; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "attrib"; }

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
   * @brief Gets the value of numComponents. AccessType: initializeOnly
   * @details numComponents specifies how many consecutive floating-point values
   * should be grouped together per vertex.
   * @return SFInt32 The current value of numComponents.
   */
  SFInt32 getNumComponents() const { return _numComponents; }
  /**
   * @brief Data-layer write of numComponents (reader/init ingest path).
   * @details numComponents is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setNumComponents().
   */
  void setNumComponentsUnchecked(const SFInt32 &value) {
    _numComponents = value;
  }

  /**
   * @brief Gets the value of value. AccessType: inputOutput
   * @details value specifies an arbitrary collection of floating-point values
   * that will be passed to the shader as per-vertex information.
   * @return MFFloat The current value of value.
   */
  MFFloat getValue() const { return _value; }

  /**
   * @brief Sets the value of value. AccessType: inputOutput
   * @details value specifies an arbitrary collection of floating-point values
   * that will be passed to the shader as per-vertex information.
   * @param value The new value for value.
   */
  void setValue(const MFFloat &value) { _value = value; }

  void setValue(MFFloat &&value) { _value = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "FloatVertexAttribute").
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

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesNumComponents(const SFInt32 &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for numComponents.
   */

  SFInt32 _numComponents{4};

  /**
   * @brief Member variable for value.
   */

  MFFloat _value{};
};

} // namespace x3d::nodes
