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
 * @class MultiTextureTransform
 * @brief MultiTextureTransform contains multiple TextureTransform nodes, each
 * provided for use by corresponding ImageTexture MovieTexture or PixelTexture
 * nodes within a sibling MultiTexture node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#MultiTextureTransform
 */
class MultiTextureTransform : public virtual X3DTextureTransformNode {
public:
  MultiTextureTransform() = default;

  ~MultiTextureTransform() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "textureTransform"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Zero or more contained TextureTransform nodes, for each of the
   * different texture channels, that define 2D transformation applied to
   * texture coordinates.
   */
  const MFNode &getTextureTransform() const { return _textureTransform; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureTransformNode
   */
  static const std::vector<std::string> &acceptableTextureTransformNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureTransformNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Zero or more contained TextureTransform nodes, for each of the
   * different texture channels, that define 2D transformation applied to
   * texture coordinates.
   */
  void setTextureTransform(const MFNode &value) { _textureTransform = value; }

  void setTextureTransform(MFNode &&value) {

    _textureTransform = std::move(value);
  }

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
  MFNode _textureTransform{};
};

} // namespace x3d::nodes
