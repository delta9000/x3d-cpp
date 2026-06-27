// X3DTextureProjectorNode.hpp
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
 * @class X3DTextureProjectorNode
 * @brief Base type for all node types that specify texture projector nodes,
 * which provide a form of lighting.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjector.html#X3DTextureProjectorNode
 */
class X3DTextureProjectorNode : public virtual X3DLightNode {
public:
  /**
   * @brief Default constructor for X3DTextureProjectorNode
   */
  X3DTextureProjectorNode() = default;

  /**
   * @brief Virtual destructor for X3DTextureProjectorNode
   */
  virtual ~X3DTextureProjectorNode() = default;

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

  /**
   * @brief Get the default value for farDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFarDistance() { return -1; }

  /**
   * @brief Get the default value for global
   * @return SFBool The default value
   */
  static SFBool getDefaultGlobal() { return true; }

  /**
   * @brief Get the default value for location
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for nearDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultNearDistance() { return -1; }

  /**
   * @brief Get the default value for texture
   * @return SFNode The default value
   */
  static SFNode getDefaultTexture() { return nullptr; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "TextureProjection"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of aspectRatio. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of aspectRatio.
   */
  SFFloat getAspectRatio() const { return _aspectRatio; }

  /**
   * @brief Emit an output value on aspectRatio. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitAspectRatio(const SFFloat &value) { _aspectRatio = value; }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * @brief Gets the value of farDistance. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of farDistance.
   */
  SFFloat getFarDistance() const { return _farDistance; }

  /**
   * @brief Sets the value of farDistance. AccessType: inputOutput
   * @details
   * @param value The new value for farDistance.
   */
  void setFarDistance(const SFFloat &value) {

    validateFarDistance(value);

    _farDistance = value;
  }

  /**
   * @brief Non-validating write of farDistance (runtime/reader ingest path).
   * @details Assigns without the range check setFarDistance() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFarDistance() stays the
   *          enforcement point for programmatic callers.
   */
  void setFarDistanceUnchecked(const SFFloat &value) { _farDistance = value; }

  /**
   * @brief Gets the value of global. AccessType: inputOutput
   * @details
   * @return SFBool The current value of global.
   */
  SFBool getGlobal() const { return _global; }

  /**
   * @brief Sets the value of global. AccessType: inputOutput
   * @details
   * @param value The new value for global.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * @brief Gets the value of location. AccessType: inputOutput
   * @details
   * @return SFVec3f The current value of location.
   */
  SFVec3f getLocation() const { return _location; }

  /**
   * @brief Sets the value of location. AccessType: inputOutput
   * @details
   * @param value The new value for location.
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * @brief Gets the value of nearDistance. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of nearDistance.
   */
  SFFloat getNearDistance() const { return _nearDistance; }

  /**
   * @brief Sets the value of nearDistance. AccessType: inputOutput
   * @details
   * @param value The new value for nearDistance.
   */
  void setNearDistance(const SFFloat &value) {

    validateNearDistance(value);

    _nearDistance = value;
  }

  /**
   * @brief Non-validating write of nearDistance (runtime/reader ingest path).
   * @details Assigns without the range check setNearDistance() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setNearDistance() stays the
   *          enforcement point for programmatic callers.
   */
  void setNearDistanceUnchecked(const SFFloat &value) { _nearDistance = value; }

  /**
   * @brief Gets the value of texture. AccessType: inputOutput
   * @details
   * @return SFNode The current value of texture.
   */
  SFNode getTexture() const { return _texture; }

  /**
   * @brief Acceptable node types for the texture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of texture. AccessType: inputOutput
   * @details
   * @param value The new value for texture.
   */
  void setTexture(const SFNode &value) { _texture = value; }

  void setTexture(SFNode &&value) { _texture = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DTextureProjectorNode").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFarDistance(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesNearDistance(const SFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

private:
  static void validateFarDistance(const SFFloat &value) {

    if (value < -1)
      throw std::out_of_range("farDistance below minimum of -1");
  }

  static void validateNearDistance(const SFFloat &value) {

    if (value < -1)
      throw std::out_of_range("nearDistance below minimum of -1");
  }

  /**
   * @brief Member variable for aspectRatio.
   */

  SFFloat _aspectRatio{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  /**
   * @brief Member variable for farDistance.
   */

  SFFloat _farDistance{-1};

  /**
   * @brief Member variable for global.
   */

  SFBool _global{true};

  /**
   * @brief Member variable for location.
   */

  SFVec3f _location{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for nearDistance.
   */

  SFFloat _nearDistance{-1};

  /**
   * @brief Member variable for texture.
   */

  SFNode _texture{nullptr};
};

} // namespace x3d::nodes
