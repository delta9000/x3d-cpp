// ShadedVolumeStyle.hpp
#ifndef SHADEDVOLUMESTYLE_HPP
#define SHADEDVOLUMESTYLE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DVolumeRenderStyleNode.hpp"

#include "X3DComposableVolumeRenderStyleNode.hpp"

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
  /**
   * @brief Default constructor for ShadedVolumeStyle
   */
  ShadedVolumeStyle() = default;

  /**
   * @brief Destructor for ShadedVolumeStyle
   */
  ~ShadedVolumeStyle() = default;

  /**
   * @brief Get the default value for lighting
   * @return SFBool The default value
   */
  static SFBool getDefaultLighting() { return false; }

  /**
   * @brief Get the default value for material
   * @return SFNode The default value
   */
  static SFNode getDefaultMaterial() { return nullptr; }

  /**
   * @brief Get the default value for phaseFunction
   * @return SFString The default value
   */
  static SFString getDefaultPhaseFunction() { return "Henyey-Greenstein"; }

  /**
   * @brief Get the default value for shadows
   * @return SFBool The default value
   */
  static SFBool getDefaultShadows() { return false; }

  /**
   * @brief Get the default value for surfaceNormals
   * @return SFNode The default value
   */
  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "renderStyle"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "VolumeRendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of lighting. AccessType: inputOutput
   * @details Whether rendering calculates and applies shading effects to visual
   * output.
   * @return SFBool The current value of lighting.
   */
  SFBool getLighting() const { return _lighting; }

  /**
   * @brief Sets the value of lighting. AccessType: inputOutput
   * @details Whether rendering calculates and applies shading effects to visual
   * output.
   * @param value The new value for lighting.
   */
  void setLighting(const SFBool &value) { _lighting = value; }

  /**
   * @brief Gets the value of material. AccessType: inputOutput
   * @details Colour and opacity is determined based on whether a value has been
   * specified for the material field.
   * @return SFNode The current value of material.
   */
  SFNode getMaterial() const { return _material; }

  /**
   * @brief Acceptable node types for the material field.
   * @details Permitted X3D node types: X3DMaterialNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * @brief Sets the value of material. AccessType: inputOutput
   * @details Colour and opacity is determined based on whether a value has been
   * specified for the material field.
   * @param value The new value for material.
   */
  void setMaterial(const SFNode &value) { _material = value; }

  void setMaterial(SFNode &&value) { _material = std::move(value); }

  /**
   * @brief Gets the value of phaseFunction. AccessType: initializeOnly
   * @details define scattering model for implementations using global
   * illumination (NONE or Henyey-Greenstein phase function).
   * @return SFString The current value of phaseFunction.
   */
  SFString getPhaseFunction() const { return _phaseFunction; }
  /**
   * @brief Data-layer write of phaseFunction (reader/init ingest path).
   * @details phaseFunction is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setPhaseFunction().
   */
  void setPhaseFunctionUnchecked(const SFString &value) {
    _phaseFunction = value;
  }

  /**
   * @brief Gets the value of shadows. AccessType: inputOutput
   * @details Whether rendering calculates and applies shadows to visual output
   * (using global illumination model).
   * @return SFBool The current value of shadows.
   */
  SFBool getShadows() const { return _shadows; }

  /**
   * @brief Sets the value of shadows. AccessType: inputOutput
   * @details Whether rendering calculates and applies shadows to visual output
   * (using global illumination model).
   * @param value The new value for shadows.
   */
  void setShadows(const SFBool &value) { _shadows = value; }

  /**
   * @brief Gets the value of surfaceNormals. AccessType: inputOutput
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   * @return SFNode The current value of surfaceNormals.
   */
  SFNode getSurfaceNormals() const { return _surfaceNormals; }

  /**
   * @brief Acceptable node types for the surfaceNormals field.
   * @details Permitted X3D node types: X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSurfaceNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of surfaceNormals. AccessType: inputOutput
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   * @param value The new value for surfaceNormals.
   */
  void setSurfaceNormals(const SFNode &value) { _surfaceNormals = value; }

  void setSurfaceNormals(SFNode &&value) { _surfaceNormals = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ShadedVolumeStyle").
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
   * @brief Member variable for lighting.
   */

  SFBool _lighting{false};

  /**
   * @brief Member variable for material.
   */

  SFNode _material{nullptr};

  /**
   * @brief Member variable for phaseFunction.
   */

  SFString _phaseFunction{"Henyey-Greenstein"};

  /**
   * @brief Member variable for shadows.
   */

  SFBool _shadows{false};

  /**
   * @brief Member variable for surfaceNormals.
   */

  SFNode _surfaceNormals{nullptr};
};

#endif // SHADEDVOLUMESTYLE_HPP