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

#include "x3d/nodes/X3DSingleTextureCoordinateNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureCoordinate3D
 * @brief TextureCoordinate3D specifies a set of 3D texture coordinates used by
 * vertex-based geometry nodes (such as IndexedFaceSet or ElevationGrid) to map
 * 3D textures to vertices.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#TextureCoordinate3D
 */
class TextureCoordinate3D : public virtual X3DSingleTextureCoordinateNode {
public:
  TextureCoordinate3D() = default;

  ~TextureCoordinate3D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "texCoord"; }

  static std::string componentName() { return "Texturing3D"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details triplets of 3D (s,t,r) texture coordinates, either in range [0,1]
   * or higher if repeating.
   */
  const MFVec3f &getPoint() const { return _point; }

  /**
   * Access: inputOutput.
   * @details triplets of 3D (s,t,r) texture coordinates, either in range [0,1]
   * or higher if repeating.
   */
  void setPoint(const MFVec3f &value) { _point = value; }

  void setPoint(MFVec3f &&value) { _point = std::move(value); }

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
  MFVec3f _point{};
};

} // namespace x3d::nodes
