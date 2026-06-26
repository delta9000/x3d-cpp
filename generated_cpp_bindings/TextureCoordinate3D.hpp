// TextureCoordinate3D.hpp
#ifndef TEXTURECOORDINATE3D_HPP
#define TEXTURECOORDINATE3D_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometricPropertyNode.hpp"

#include "X3DTextureCoordinateNode.hpp"

#include "X3DSingleTextureCoordinateNode.hpp"

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
  /**
   * @brief Default constructor for TextureCoordinate3D
   */
  TextureCoordinate3D() = default;

  /**
   * @brief Destructor for TextureCoordinate3D
   */
  ~TextureCoordinate3D() = default;

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
  static std::string componentName() { return "Texturing3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of point. AccessType: inputOutput
   * @details triplets of 3D (s,t,r) texture coordinates, either in range [0,1]
   * or higher if repeating.
   * @return MFVec3f The current value of point.
   */
  MFVec3f getPoint() const { return _point; }

  /**
   * @brief Sets the value of point. AccessType: inputOutput
   * @details triplets of 3D (s,t,r) texture coordinates, either in range [0,1]
   * or higher if repeating.
   * @param value The new value for point.
   */
  void setPoint(const MFVec3f &value) { _point = value; }

  void setPoint(MFVec3f &&value) { _point = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureCoordinate3D").
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
   * @brief Member variable for point.
   */

  MFVec3f _point{};
};

#endif // TEXTURECOORDINATE3D_HPP