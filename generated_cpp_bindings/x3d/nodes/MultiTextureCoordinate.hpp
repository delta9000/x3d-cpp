// MultiTextureCoordinate.hpp
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
  /**
   * @brief Default constructor for MultiTextureCoordinate
   */
  MultiTextureCoordinate() = default;

  /**
   * @brief Destructor for MultiTextureCoordinate
   */
  ~MultiTextureCoordinate() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "texCoord"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of texCoord. AccessType: inputOutput
   * @details Zero or more contained TextureCoordinate or
   * TextureCoordinateGenerator nodes that specify texture coordinates for the
   * different texture channels, used for texture mapping onto corresponding
   * geometry.
   * @return const MFNode& The current value of texCoord.
   */
  const MFNode &getTexCoord() const { return _texCoord; }

  /**
   * @brief Acceptable node types for the texCoord field.
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode"};
    return types;
  }

  /**
   * @brief Sets the value of texCoord. AccessType: inputOutput
   * @details Zero or more contained TextureCoordinate or
   * TextureCoordinateGenerator nodes that specify texture coordinates for the
   * different texture channels, used for texture mapping onto corresponding
   * geometry.
   * @param value The new value for texCoord.
   */
  void setTexCoord(const MFNode &value) { _texCoord = value; }

  void setTexCoord(MFNode &&value) { _texCoord = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MultiTextureCoordinate").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

private:
  /**
   * @brief Member variable for texCoord.
   */

  MFNode _texCoord{};
};

} // namespace x3d::nodes
