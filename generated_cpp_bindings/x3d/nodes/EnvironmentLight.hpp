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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class EnvironmentLight
 * @brief EnvironmentLight node uses an environment map to represent incident
 * illumination around a point, and can be used to show reflections of distant
 * objects.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/lighting.html#EnvironmentLight
 */
class EnvironmentLight : public virtual X3DLightNode {
public:
  EnvironmentLight() = default;

  ~EnvironmentLight() = default;

  static SFNode getDefaultDiffuseTexture() { return nullptr; }

  static SFBool getDefaultGlobal() { return true; }

  static SFVec3f getDefaultOrigin() { return SFVec3f{0, 0, 0}; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFNode getDefaultSpecularTexture() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Lighting"; }

  static int componentLevel() { return 1; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details diffuseCoefficients field provides a 3 x 9 array of float values
   * providing spherical harmonic coefficients for low-frequency characteristics
   * of the environment map to produce an irradiance map corresponding to glTF
   * irradianceCoefficients field.
   */
  const MFFloat &getDiffuseCoefficients() const { return _diffuseCoefficients; }

  /**
   * Access: inputOutput.
   * @details diffuseCoefficients field provides a 3 x 9 array of float values
   * providing spherical harmonic coefficients for low-frequency characteristics
   * of the environment map to produce an irradiance map corresponding to glTF
   * irradianceCoefficients field.
   */
  void setDiffuseCoefficients(const MFFloat &value) {

    _diffuseCoefficients = value;
  }

  void setDiffuseCoefficients(MFFloat &&value) {

    _diffuseCoefficients = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details diffuseTexture defines explicit precomputed
   * X3DEnvironmentTextureNode (ComposedCubeMapTexture, GeneratedCubeMapTexture,
   * ImageCubeMapTexture) nodes as the image source for the EnvironmentLight.
   */
  const SFNode &getDiffuseTexture() const { return _diffuseTexture; }

  /**
   * @details Permitted X3D node types: X3DEnvironmentTextureNode
   */
  static const std::vector<std::string> &acceptableDiffuseTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DEnvironmentTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details diffuseTexture defines explicit precomputed
   * X3DEnvironmentTextureNode (ComposedCubeMapTexture, GeneratedCubeMapTexture,
   * ImageCubeMapTexture) nodes as the image source for the EnvironmentLight.
   */
  void setDiffuseTexture(const SFNode &value) { _diffuseTexture = value; }

  void setDiffuseTexture(SFNode &&value) { _diffuseTexture = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details global field affects the scope of lighting effects produced by the
   * EnvironmentLight node, and has no effect on the computation of environment
   * textures.
   */
  const SFBool &getGlobal() const { return _global; }

  /**
   * Access: inputOutput.
   * @details global field affects the scope of lighting effects produced by the
   * EnvironmentLight node, and has no effect on the computation of environment
   * textures.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details origin defines the relative position for observing the surrounding
   * scene to create an environment texture.
   */
  const SFVec3f &getOrigin() const { return _origin; }

  /**
   * Access: inputOutput.
   * @details origin defines the relative position for observing the surrounding
   * scene to create an environment texture.
   */
  void setOrigin(const SFVec3f &value) { _origin = value; }

  void setOrigin(SFVec3f &&value) { _origin = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details rotation field represents the overall rotation of the IBL
   * environment.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details rotation field represents the overall rotation of the IBL
   * environment.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details specularTexture defines explicit precomputed
   * X3DEnvironmentTextureNode (ComposedCubeMapTexture, GeneratedCubeMapTexture,
   * ImageCubeMapTexture) nodes as the image source for the EnvironmentLight.
   */
  const SFNode &getSpecularTexture() const { return _specularTexture; }

  /**
   * @details Permitted X3D node types: X3DEnvironmentTextureNode
   */
  static const std::vector<std::string> &acceptableSpecularTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DEnvironmentTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details specularTexture defines explicit precomputed
   * X3DEnvironmentTextureNode (ComposedCubeMapTexture, GeneratedCubeMapTexture,
   * ImageCubeMapTexture) nodes as the image source for the EnvironmentLight.
   */
  void setSpecularTexture(const SFNode &value) { _specularTexture = value; }

  void setSpecularTexture(SFNode &&value) {

    _specularTexture = std::move(value);
  }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  MFFloat _diffuseCoefficients{};

  SFNode _diffuseTexture{nullptr};

  SFBool _global{true};

  SFVec3f _origin{SFVec3f{0, 0, 0}};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFNode _specularTexture{nullptr};
};

} // namespace x3d::nodes
