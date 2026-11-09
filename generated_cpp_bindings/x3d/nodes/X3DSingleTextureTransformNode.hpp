#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DTextureTransformNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSingleTextureTransformNode
 * @brief Base type for all texture transform nodes which specify texture
 * coordinate transformation for a single texture.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#X3DSingleTextureTransformNode
 */
class X3DSingleTextureTransformNode : public virtual X3DTextureTransformNode {
public:
  X3DSingleTextureTransformNode() = default;

  virtual ~X3DSingleTextureTransformNode() = default;

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
