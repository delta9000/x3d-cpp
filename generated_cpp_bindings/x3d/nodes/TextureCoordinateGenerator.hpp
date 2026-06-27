// TextureCoordinateGenerator.hpp
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
 * @class TextureCoordinateGenerator
 * @brief TextureCoordinateGenerator computes 2D (s,t) texture-coordinate
 * points, used by vertex-based geometry nodes (such as IndexedFaceSet or
 * ElevationGrid) to map textures to vertices (and patches to NURBS surfaces).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#TextureCoordinateGenerator
 */
class TextureCoordinateGenerator
    : public virtual X3DSingleTextureCoordinateNode {
public:
  /**
   * @brief Default constructor for TextureCoordinateGenerator
   */
  TextureCoordinateGenerator() = default;

  /**
   * @brief Destructor for TextureCoordinateGenerator
   */
  ~TextureCoordinateGenerator() = default;

  /**
   * @brief Get the default value for mode
   * @return TextureCoordinateGeneratorModeChoices The default value
   */
  static TextureCoordinateGeneratorModeChoices getDefaultMode() {
    return TextureCoordinateGeneratorModeChoices::SPHERE;
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesTextureCoordinate";
  }

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
   * @brief Gets the value of mode. AccessType: inputOutput
   * @details parameter field defines the algorithm used to compute texture
   * coordinates.
   * @return TextureCoordinateGeneratorModeChoices The current value of mode.
   */
  TextureCoordinateGeneratorModeChoices getMode() const { return _mode; }

  /**
   * @brief Sets the value of mode. AccessType: inputOutput
   * @details parameter field defines the algorithm used to compute texture
   * coordinates.
   * @param value The new value for mode.
   */
  void setMode(const TextureCoordinateGeneratorModeChoices &value) {

    _mode = value;
  }

  /**
   * @brief Gets the value of parameter. AccessType: inputOutput
   * @details parameter array contains scale and translation (x y z) values for
   * Perlin NOISE mode, parameter[0] contains index of refraction for
   * SPHERE-REFLECT mode, parameter[0] contains index of refraction and
   * parameter[1 to 3] contains the eye point in local coordinates for
   * SPHERE-REFLECT-LOCAL mode.
   * @return MFFloat The current value of parameter.
   */
  MFFloat getParameter() const { return _parameter; }

  /**
   * @brief Sets the value of parameter. AccessType: inputOutput
   * @details parameter array contains scale and translation (x y z) values for
   * Perlin NOISE mode, parameter[0] contains index of refraction for
   * SPHERE-REFLECT mode, parameter[0] contains index of refraction and
   * parameter[1 to 3] contains the eye point in local coordinates for
   * SPHERE-REFLECT-LOCAL mode.
   * @param value The new value for parameter.
   */
  void setParameter(const MFFloat &value) { _parameter = value; }

  void setParameter(MFFloat &&value) { _parameter = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureCoordinateGenerator").
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
   * @brief Member variable for mode.
   */

  TextureCoordinateGeneratorModeChoices _mode{
      TextureCoordinateGeneratorModeChoices::SPHERE};

  /**
   * @brief Member variable for parameter.
   */

  MFFloat _parameter{};
};

} // namespace x3d::nodes
