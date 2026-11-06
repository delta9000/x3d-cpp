#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DVolumeRenderStyleNode.hpp"

#include "x3d/nodes/X3DComposableVolumeRenderStyleNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ShadedVolumeStyle
 * @brief All fields fully supported except shadows supported with at least
 * Phong shading at level 3. All fields fully supported with at least Phong
 * shading and Henyey-Greenstein phase function, shadows fully supported at
 * level 4.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#ShadedVolumeStyle
 */
class ShadedVolumeStyle : public virtual X3DComposableVolumeRenderStyleNode {
public:
  ShadedVolumeStyle() = default;

  ~ShadedVolumeStyle() = default;

  static SFBool getDefaultLighting() { return false; }

  static SFNode getDefaultMaterial() { return nullptr; }

  static SFString getDefaultPhaseFunction() { return "Henyey-Greenstein"; }

  static SFBool getDefaultShadows() { return false; }

  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 3; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether rendering calculates and applies shading effects to visual
   * output.
   */
  const SFBool &getLighting() const { return _lighting; }

  /**
   * Access: inputOutput.
   * @details Whether rendering calculates and applies shading effects to visual
   * output.
   */
  void setLighting(const SFBool &value) { _lighting = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Colour and opacity is determined based on whether a value has been
   * specified for the material field.
   */
  const SFNode &getMaterial() const { return _material; }

  /**
   * @details Permitted X3D node types: X3DMaterialNode
   */
  static const std::vector<std::string> &acceptableMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Colour and opacity is determined based on whether a value has been
   * specified for the material field.
   */
  void setMaterial(const SFNode &value) { _material = value; }

  void setMaterial(SFNode &&value) { _material = std::move(value); }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details define scattering model for implementations using global
   * illumination (NONE or Henyey-Greenstein phase function).
   */
  const SFString &getPhaseFunction() const { return _phaseFunction; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setPhaseFunctionUnchecked(const SFString &value) {
    _phaseFunction = value;
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether rendering calculates and applies shadows to visual output
   * (using global illumination model).
   */
  const SFBool &getShadows() const { return _shadows; }

  /**
   * Access: inputOutput.
   * @details Whether rendering calculates and applies shadows to visual output
   * (using global illumination model).
   */
  void setShadows(const SFBool &value) { _shadows = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   */
  const SFNode &getSurfaceNormals() const { return _surfaceNormals; }

  /**
   * @details Permitted X3D node types: X3DTexture3DNode
   */
  static const std::vector<std::string> &acceptableSurfaceNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   */
  void setSurfaceNormals(const SFNode &value) { _surfaceNormals = value; }

  void setSurfaceNormals(SFNode &&value) { _surfaceNormals = std::move(value); }

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
  SFBool _lighting{false};

  SFNode _material{nullptr};

  SFString _phaseFunction{"Henyey-Greenstein"};

  SFBool _shadows{false};

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
