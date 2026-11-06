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
 * @class MultiTextureCoordinate
 * @brief MultiTextureCoordinate contains multiple TextureCoordinate or
 * TextureCoordinateGenerator nodes, for use by a parent polygonal geometry node
 * such as IndexedFaceSet or a Triangle* node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#MultiTextureCoordinate
 */
class MultiTextureCoordinate : public virtual X3DTextureCoordinateNode {
public:
  MultiTextureCoordinate() = default;

  ~MultiTextureCoordinate() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "texCoord"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Zero or more contained TextureCoordinate or
   * TextureCoordinateGenerator nodes that specify texture coordinates for the
   * different texture channels, used for texture mapping onto corresponding
   * geometry.
   */
  const MFNode &getTexCoord() const { return _texCoord; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Zero or more contained TextureCoordinate or
   * TextureCoordinateGenerator nodes that specify texture coordinates for the
   * different texture channels, used for texture mapping onto corresponding
   * geometry.
   */
  void setTexCoord(const MFNode &value) { _texCoord = value; }

  void setTexCoord(MFNode &&value) { _texCoord = std::move(value); }

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
  MFNode _texCoord{};
};

} // namespace x3d::nodes
