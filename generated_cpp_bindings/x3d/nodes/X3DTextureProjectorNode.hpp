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
  X3DTextureProjectorNode() = default;

  virtual ~X3DTextureProjectorNode() = default;

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

  static SFFloat getDefaultFarDistance() { return -1; }

  static SFBool getDefaultGlobal() { return true; }

  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultNearDistance() { return -1; }

  static SFNode getDefaultTexture() { return nullptr; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "TextureProjection"; }

  static int componentLevel() { return 4; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getAspectRatio() const { return _aspectRatio; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitAspectRatio(const SFFloat &value) { _aspectRatio = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getFarDistance() const { return _farDistance; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setFarDistance(const SFFloat &value) {

    validateFarDistance(value);

    _farDistance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFarDistance() for range validation.
   */
  void setFarDistanceUnchecked(const SFFloat &value) { _farDistance = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getGlobal() const { return _global; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFVec3f &getLocation() const { return _location; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getNearDistance() const { return _nearDistance; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setNearDistance(const SFFloat &value) {

    validateNearDistance(value);

    _nearDistance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setNearDistance() for range validation.
   */
  void setNearDistanceUnchecked(const SFFloat &value) { _nearDistance = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getTexture() const { return _texture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTexture(const SFNode &value) { _texture = value; }

  void setTexture(SFNode &&value) { _texture = std::move(value); }

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

    if (value < -1.0f)
      throw std::out_of_range("farDistance below minimum of -1");
  }

  static void validateNearDistance(const SFFloat &value) {

    if (value < -1.0f)
      throw std::out_of_range("nearDistance below minimum of -1");
  }

  SFFloat _aspectRatio{};

  SFString _description{};

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  SFFloat _farDistance{-1};

  SFBool _global{true};

  SFVec3f _location{SFVec3f{0, 0, 0}};

  SFFloat _nearDistance{-1};

  SFNode _texture{nullptr};
};

} // namespace x3d::nodes
