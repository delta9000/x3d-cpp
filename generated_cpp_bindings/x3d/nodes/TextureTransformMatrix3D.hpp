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

#include "x3d/nodes/X3DSingleTextureTransformNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureTransformMatrix3D
 * @brief TextureTransformMatrix3D applies a 3D transformation to texture
 * coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/texture3D.html#TextureTransformMatrix3D
 */
class TextureTransformMatrix3D : public virtual X3DSingleTextureTransformNode {
public:
  TextureTransformMatrix3D() = default;

  ~TextureTransformMatrix3D() = default;

  static SFMatrix4f getDefaultMatrix() {
    return SFMatrix4f{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "textureTransform"; }

  static std::string componentName() { return "Texturing3D"; }

  static int componentLevel() { return 1; }

  /**
   * SFMatrix4f; access: inputOutput. Borrows the stored value.
   * @details matrix is a generalized, unfiltered 4x4 transformation matrix to
   * modify texture (opposite effect appears on geometry).
   */
  const SFMatrix4f &getMatrix() const { return _matrix; }

  /**
   * Access: inputOutput.
   * @details matrix is a generalized, unfiltered 4x4 transformation matrix to
   * modify texture (opposite effect appears on geometry).
   */
  void setMatrix(const SFMatrix4f &value) { _matrix = value; }

  void setMatrix(SFMatrix4f &&value) { _matrix = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  SFMatrix4f _matrix{
      SFMatrix4f{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}};
};

} // namespace x3d::nodes
