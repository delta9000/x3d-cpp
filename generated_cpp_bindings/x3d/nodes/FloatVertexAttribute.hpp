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
  FloatVertexAttribute() = default;

  ~FloatVertexAttribute() = default;

  static SFInt32 getDefaultNumComponents() { return 4; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "attrib"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details numComponents specifies how many consecutive floating-point values
   * should be grouped together per vertex.
   */
  const SFInt32 &getNumComponents() const { return _numComponents; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setNumComponentsUnchecked(const SFInt32 &value) {
    _numComponents = value;
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details value specifies an arbitrary collection of floating-point values
   * that will be passed to the shader as per-vertex information.
   */
  const MFFloat &getValue() const { return _value; }

  /**
   * Access: inputOutput.
   * @details value specifies an arbitrary collection of floating-point values
   * that will be passed to the shader as per-vertex information.
   */
  void setValue(const MFFloat &value) { _value = value; }

  void setValue(MFFloat &&value) { _value = std::move(value); }

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
  SFInt32 _numComponents{4};

  MFFloat _value{};
};

} // namespace x3d::nodes
