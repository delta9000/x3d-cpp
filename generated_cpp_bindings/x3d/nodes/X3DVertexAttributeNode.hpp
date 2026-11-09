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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DVertexAttributeNode
 * @brief Base type for all nodes that specify per-vertex attribute information
 * to the shader.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#X3DVertexAttributeNode
 */
class X3DVertexAttributeNode : public virtual X3DGeometricPropertyNode {
public:
  X3DVertexAttributeNode() = default;

  virtual ~X3DVertexAttributeNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * xs_nmtoken; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const xs_nmtoken &getName() const { return _name; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setNameUnchecked(const xs_nmtoken &value) { _name = value; }

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
  xs_nmtoken _name{};
};

} // namespace x3d::nodes
