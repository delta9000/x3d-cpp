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

#include "x3d/nodes/X3DLightNode.hpp"

#include "x3d/nodes/X3DTextureProjectorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureProjectorParallel
 * @brief TextureProjectorParallel is similar to a light that projects a texture
 * into the scene, illuminating geometry that intersects the parallel projection
 * volume.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjector.html#TextureProjectorParallel
 */
class TextureProjectorParallel : public virtual X3DTextureProjectorNode {
public:
  TextureProjectorParallel() = default;

  ~TextureProjectorParallel() = default;

  static SFVec4f getDefaultFieldOfView() { return SFVec4f{-1, -1, 1, 1}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "TextureProjection"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec4f; access: inputOutput. Borrows the stored value.
   * @details Minimum and maximum extents of projection texture in units of
   * local coordinate system.
   */
  const SFVec4f &getFieldOfView() const { return _fieldOfView; }

  /**
   * Access: inputOutput.
   * @details Minimum and maximum extents of projection texture in units of
   * local coordinate system.
   */
  void setFieldOfView(const SFVec4f &value) { _fieldOfView = value; }

  void setFieldOfView(SFVec4f &&value) { _fieldOfView = std::move(value); }

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

private:
  SFVec4f _fieldOfView{SFVec4f{-1, -1, 1, 1}};
};

} // namespace x3d::nodes
