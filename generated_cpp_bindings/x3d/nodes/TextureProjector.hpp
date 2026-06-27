// TextureProjector.hpp
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
  /**
   * @brief Default constructor for TextureProjector
   */
  TextureProjector() = default;

  /**
   * @brief Destructor for TextureProjector
   */
  ~TextureProjector() = default;

  /**
   * @brief Get the default value for fieldOfView
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFieldOfView() { return 0.7854; }

  /**
   * @brief Get the default value for upVector
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultUpVector() { return SFVec3f{0, 0, 1}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of fieldOfView. AccessType: inputOutput
   * @details Preferred minimum viewing angle for this projection in radians,
   * providing minimum height or minimum width (whichever is smaller).
   * @return SFFloat The current value of fieldOfView.
   */
  SFFloat getFieldOfView() const { return _fieldOfView; }

  /**
   * @brief Sets the value of fieldOfView. AccessType: inputOutput
   * @details Preferred minimum viewing angle for this projection in radians,
   * providing minimum height or minimum width (whichever is smaller).
   * @param value The new value for fieldOfView.
   */
  void setFieldOfView(const SFFloat &value) {

    validateFieldOfView(value);

    _fieldOfView = value;
  }

  /**
   * @brief Non-validating write of fieldOfView (runtime/reader ingest path).
   * @details Assigns without the range check setFieldOfView() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFieldOfView() stays the
   *          enforcement point for programmatic callers.
   */
  void setFieldOfViewUnchecked(const SFFloat &value) { _fieldOfView = value; }

  /**
   * @brief Gets the value of upVector. AccessType: inputOutput
   * @details upVector describes the roll of the camera by saying which
   * direction is up for the camera's orientation.
   * @return SFVec3f The current value of upVector.
   */
  SFVec3f getUpVector() const { return _upVector; }

  /**
   * @brief Sets the value of upVector. AccessType: inputOutput
   * @details upVector describes the roll of the camera by saying which
   * direction is up for the camera's orientation.
   * @param value The new value for upVector.
   */
  void setUpVector(const SFVec3f &value) { _upVector = value; }

  void setUpVector(SFVec3f &&value) { _upVector = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureProjector").
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
  static void checkRangesFieldOfView(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateFieldOfView(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("fieldOfView below minimum of 0");
    if (value > 3.1416)
      throw std::out_of_range("fieldOfView above maximum of 3.1416");
  }

  /**
   * @brief Member variable for fieldOfView.
   */

  SFFloat _fieldOfView{0.7854};

  /**
   * @brief Member variable for upVector.
   */

  SFVec3f _upVector{SFVec3f{0, 0, 1}};
};

} // namespace x3d::nodes
