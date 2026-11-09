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
  TextureCoordinateGenerator() = default;

  ~TextureCoordinateGenerator() = default;

  static TextureCoordinateGeneratorModeChoices getDefaultMode() {
    return TextureCoordinateGeneratorModeChoices::SPHERE;
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesTextureCoordinate";
  }

  static std::string getDefaultContainerField() { return "texCoord"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 2; }

  /**
   * TextureCoordinateGeneratorModeChoices; access: inputOutput. Borrows the
   * stored value.
   * @details parameter field defines the algorithm used to compute texture
   * coordinates.
   */
  const TextureCoordinateGeneratorModeChoices &getMode() const { return _mode; }

  /**
   * Access: inputOutput.
   * @details parameter field defines the algorithm used to compute texture
   * coordinates.
   */
  void setMode(const TextureCoordinateGeneratorModeChoices &value) {

    _mode = value;
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details parameter array contains scale and translation (x y z) values for
   * Perlin NOISE mode, parameter[0] contains index of refraction for
   * SPHERE-REFLECT mode, parameter[0] contains index of refraction and
   * parameter[1 to 3] contains the eye point in local coordinates for
   * SPHERE-REFLECT-LOCAL mode.
   */
  const MFFloat &getParameter() const { return _parameter; }

  /**
   * Access: inputOutput.
   * @details parameter array contains scale and translation (x y z) values for
   * Perlin NOISE mode, parameter[0] contains index of refraction for
   * SPHERE-REFLECT mode, parameter[0] contains index of refraction and
   * parameter[1 to 3] contains the eye point in local coordinates for
   * SPHERE-REFLECT-LOCAL mode.
   */
  void setParameter(const MFFloat &value) { _parameter = value; }

  void setParameter(MFFloat &&value) { _parameter = std::move(value); }

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
  TextureCoordinateGeneratorModeChoices _mode{
      TextureCoordinateGeneratorModeChoices::SPHERE};

  MFFloat _parameter{};
};

} // namespace x3d::nodes
