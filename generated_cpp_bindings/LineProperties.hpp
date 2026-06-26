// LineProperties.hpp
#ifndef LINEPROPERTIES_HPP
#define LINEPROPERTIES_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DAppearanceChildNode.hpp"

/**
 * @class LineProperties
 * @brief LineProperties allows precise fine-grained control over the rendering
 * style of lines and edges for associated geometry nodes inside the same Shape.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#LineProperties
 */
class LineProperties : public virtual X3DAppearanceChildNode {
public:
  /**
   * @brief Default constructor for LineProperties
   */
  LineProperties() = default;

  /**
   * @brief Destructor for LineProperties
   */
  ~LineProperties() = default;

  /**
   * @brief Get the default value for applied
   * @return SFBool The default value
   */
  static SFBool getDefaultApplied() { return true; }

  /**
   * @brief Get the default value for linetype
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultLinetype() { return 1; }

  /**
   * @brief Get the default value for linewidthScaleFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultLinewidthScaleFactor() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "lineProperties"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of applied. AccessType: inputOutput
   * @details Whether or not LineProperties are applied to associated geometry.
   * @return SFBool The current value of applied.
   */
  SFBool getApplied() const { return _applied; }

  /**
   * @brief Sets the value of applied. AccessType: inputOutput
   * @details Whether or not LineProperties are applied to associated geometry.
   * @param value The new value for applied.
   */
  void setApplied(const SFBool &value) { _applied = value; }

  /**
   * @brief Gets the value of linetype. AccessType: inputOutput
   * @details linetype selects a line pattern, with solid default if defined
   * value isn't supported.
   * @return SFInt32 The current value of linetype.
   */
  SFInt32 getLinetype() const { return _linetype; }

  /**
   * @brief Sets the value of linetype. AccessType: inputOutput
   * @details linetype selects a line pattern, with solid default if defined
   * value isn't supported.
   * @param value The new value for linetype.
   */
  void setLinetype(const SFInt32 &value) {

    validateLinetype(value);

    _linetype = value;
  }

  /**
   * @brief Non-validating write of linetype (runtime/reader ingest path).
   * @details Assigns without the range check setLinetype() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLinetype() stays the
   *          enforcement point for programmatic callers.
   */
  void setLinetypeUnchecked(const SFInt32 &value) { _linetype = value; }

  /**
   * @brief Gets the value of linewidthScaleFactor. AccessType: inputOutput
   * @details linewidthScaleFactor is a scale factor multiplied by
   * browser-dependent nominal linewidth, mapped to nearest available line
   * width.
   * @return SFFloat The current value of linewidthScaleFactor.
   */
  SFFloat getLinewidthScaleFactor() const { return _linewidthScaleFactor; }

  /**
   * @brief Sets the value of linewidthScaleFactor. AccessType: inputOutput
   * @details linewidthScaleFactor is a scale factor multiplied by
   * browser-dependent nominal linewidth, mapped to nearest available line
   * width.
   * @param value The new value for linewidthScaleFactor.
   */
  void setLinewidthScaleFactor(const SFFloat &value) {

    _linewidthScaleFactor = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "LineProperties").
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
  static void checkRangesLinetype(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateLinetype(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("linetype below minimum of 0");
  }

  /**
   * @brief Member variable for applied.
   */

  SFBool _applied{true};

  /**
   * @brief Member variable for linetype.
   */

  SFInt32 _linetype{1};

  /**
   * @brief Member variable for linewidthScaleFactor.
   */

  SFFloat _linewidthScaleFactor{0};
};

#endif // LINEPROPERTIES_HPP