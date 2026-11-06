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

#include "x3d/nodes/X3DTextureCoordinateNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSingleTextureCoordinateNode
 * @brief Base type for all texture coordinate nodes which specify texture
 * coordinates for a single texture.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#X3DSingleTextureCoordinateNode
 */
class X3DSingleTextureCoordinateNode : public virtual X3DTextureCoordinateNode {
public:
  X3DSingleTextureCoordinateNode() = default;

  virtual ~X3DSingleTextureCoordinateNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getMapping() const { return _mapping; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setMapping(const SFString &value) { _mapping = value; }

  void setMapping(SFString &&value) { _mapping = std::move(value); }

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
  SFString _mapping{};
};

} // namespace x3d::nodes
