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

#include "x3d/nodes/X3DTextureProjectorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureProjector
 * @brief TextureProjector is similar to a light that projects a texture into
 * the scene, illuminating geometry that intersects the perspective projection
 * volume.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjector.html#TextureProjector
 */
class TextureProjector : public virtual X3DTextureProjectorNode {
public:
  TextureProjector() = default;

  ~TextureProjector() = default;

  static SFFloat getDefaultFieldOfView() { return 0.7854; }

  static SFVec3f getDefaultUpVector() { return SFVec3f{0, 0, 1}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "TextureProjection"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Preferred minimum viewing angle for this projection in radians,
   * providing minimum height or minimum width (whichever is smaller).
   */
  const SFFloat &getFieldOfView() const { return _fieldOfView; }

  /**
   * Access: inputOutput.
   * @details Preferred minimum viewing angle for this projection in radians,
   * providing minimum height or minimum width (whichever is smaller).
   */
  void setFieldOfView(const SFFloat &value) {

    validateFieldOfView(value);

    _fieldOfView = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFieldOfView() for range validation.
   */
  void setFieldOfViewUnchecked(const SFFloat &value) { _fieldOfView = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details upVector describes the roll of the camera by saying which
   * direction is up for the camera's orientation.
   */
  const SFVec3f &getUpVector() const { return _upVector; }

  /**
   * Access: inputOutput.
   * @details upVector describes the roll of the camera by saying which
   * direction is up for the camera's orientation.
   */
  void setUpVector(const SFVec3f &value) { _upVector = value; }

  void setUpVector(SFVec3f &&value) { _upVector = std::move(value); }

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
  static void checkRangesFieldOfView(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateFieldOfView(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("fieldOfView below minimum of 0");
    if (value > 3.1416f)
      throw std::out_of_range("fieldOfView above maximum of 3.1416");
  }

  SFFloat _fieldOfView{0.7854};

  SFVec3f _upVector{SFVec3f{0, 0, 1}};
};

} // namespace x3d::nodes
