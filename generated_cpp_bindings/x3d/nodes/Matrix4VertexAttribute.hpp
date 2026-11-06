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
 * @class Matrix4VertexAttribute
 * @brief Matrix4VertexAttribute defines a set of per-vertex 4x4 matrix
 * attributes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#Matrix4VertexAttribute
 */
class Matrix4VertexAttribute : public virtual X3DVertexAttributeNode {
public:
  Matrix4VertexAttribute() = default;

  ~Matrix4VertexAttribute() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "attrib"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * MFMatrix4f; access: inputOutput. Borrows the stored value.
   * @details value specifies an arbitrary collection of matrix values that will
   * be passed to the shader as per-vertex information.
   */
  const MFMatrix4f &getValue() const { return _value; }

  /**
   * Access: inputOutput.
   * @details value specifies an arbitrary collection of matrix values that will
   * be passed to the shader as per-vertex information.
   */
  void setValue(const MFMatrix4f &value) { _value = value; }

  void setValue(MFMatrix4f &&value) { _value = std::move(value); }

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
  MFMatrix4f _value{};
};

} // namespace x3d::nodes
