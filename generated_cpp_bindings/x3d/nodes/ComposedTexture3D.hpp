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

#include "x3d/nodes/X3DTextureNode.hpp"

#include "x3d/nodes/X3DTexture3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ComposedTexture3D
 * @brief ComposedTexture3D defines a 3D image-based texture map as a collection
 * of 2D texture sources at various depths.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#ComposedTexture3D
 */
class ComposedTexture3D : public virtual X3DTexture3DNode {
public:
  ComposedTexture3D() = default;

  ~ComposedTexture3D() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture3DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing3D"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details collection of 2D texture sources.
   */
  const MFNode &getTexture() const { return _texture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details collection of 2D texture sources.
   */
  void setTexture(const MFNode &value) { _texture = value; }

  void setTexture(MFNode &&value) { _texture = std::move(value); }

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
  MFNode _texture{};
};

} // namespace x3d::nodes
