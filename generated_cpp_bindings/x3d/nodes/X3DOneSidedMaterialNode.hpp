// X3DOneSidedMaterialNode.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DMaterialNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DOneSidedMaterialNode
 * @brief Base type for material nodes that describe how the shape looks like
 * from one side. A different number of contanied texture nodes are allowed by
 * each of the implementing nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#X3DOneSidedMaterialNode
 */
class X3DOneSidedMaterialNode : public virtual X3DMaterialNode {
public:
  /**
   * @brief Default constructor for X3DOneSidedMaterialNode
   */
  X3DOneSidedMaterialNode() = default;

  /**
   * @brief Virtual destructor for X3DOneSidedMaterialNode
   */
  virtual ~X3DOneSidedMaterialNode() = default;

  /**
   * @brief Get the default value for normalScale
   * @return SFFloat The default value
   */
  static SFFloat getDefaultNormalScale() { return 1; }

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
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of emissiveTextureMapping. AccessType: inputOutput
   * @details
   * @return SFString The current value of emissiveTextureMapping.
   */
  SFString getEmissiveTextureMapping() const { return _emissiveTextureMapping; }

  /**
   * @brief Sets the value of emissiveTextureMapping. AccessType: inputOutput
   * @details
   * @param value The new value for emissiveTextureMapping.
   */
  void setEmissiveTextureMapping(const SFString &value) {

    _emissiveTextureMapping = value;
  }

  void setEmissiveTextureMapping(SFString &&value) {

    _emissiveTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of normalScale. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of normalScale.
   */
  SFFloat getNormalScale() const { return _normalScale; }

  /**
   * @brief Sets the value of normalScale. AccessType: inputOutput
   * @details
   * @param value The new value for normalScale.
   */
  void setNormalScale(const SFFloat &value) {

    validateNormalScale(value);

    _normalScale = value;
  }

  /**
   * @brief Non-validating write of normalScale (runtime/reader ingest path).
   * @details Assigns without the range check setNormalScale() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setNormalScale() stays the
   *          enforcement point for programmatic callers.
   */
  void setNormalScaleUnchecked(const SFFloat &value) { _normalScale = value; }

  /**
   * @brief Gets the value of normalTextureMapping. AccessType: inputOutput
   * @details
   * @return SFString The current value of normalTextureMapping.
   */
  SFString getNormalTextureMapping() const { return _normalTextureMapping; }

  /**
   * @brief Sets the value of normalTextureMapping. AccessType: inputOutput
   * @details
   * @param value The new value for normalTextureMapping.
   */
  void setNormalTextureMapping(const SFString &value) {

    _normalTextureMapping = value;
  }

  void setNormalTextureMapping(SFString &&value) {

    _normalTextureMapping = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "X3DOneSidedMaterialNode").
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
  static void checkRangesNormalScale(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateNormalScale(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("normalScale below minimum of 0");
  }

  /**
   * @brief Member variable for emissiveTextureMapping.
   */

  SFString _emissiveTextureMapping{};

  /**
   * @brief Member variable for normalScale.
   */

  SFFloat _normalScale{1};

  /**
   * @brief Member variable for normalTextureMapping.
   */

  SFString _normalTextureMapping{};
};

} // namespace x3d::nodes
