#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DFontStyleNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ScreenFontStyle
 * @brief ScreenFontStyle is an X3DFontStyleNode defines the size, family,
 * justification, and other styles used within a screen layout.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#ScreenFontStyle
 */
class ScreenFontStyle : public virtual X3DFontStyleNode {
public:
  ScreenFontStyle() = default;

  ~ScreenFontStyle() = default;

  static MFString getDefaultFamily() {
    return std::vector<std::string>{"SERIF"};
  }

  static SFBool getDefaultHorizontal() { return true; }

  static std::vector<JustifyChoices> getDefaultJustify() {
    return std::vector<JustifyChoices>{JustifyChoices::BEGIN};
  }

  static SFBool getDefaultLeftToRight() { return true; }

  static SFFloat getDefaultPointSize() { return 12.0; }

  static SFFloat getDefaultSpacing() { return 1.0; }

  static FontStyleChoices getDefaultStyle() { return FontStyleChoices::PLAIN; }

  static SFBool getDefaultTopToBottom() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "fontStyle"; }

  static std::string componentName() { return "Layout"; }

  static int componentLevel() { return 2; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   */
  const MFString &getFamily() const { return _family; }

  /**
   * Access: inputOutput.
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   */
  void setFamily(const MFString &value) { _family = value; }

  void setFamily(MFString &&value) { _family = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether text direction is horizontal (true) or vertical (false).
   */
  const SFBool &getHorizontal() const { return _horizontal; }

  /**
   * Access: inputOutput.
   * @details Whether text direction is horizontal (true) or vertical (false).
   */
  void setHorizontal(const SFBool &value) { _horizontal = value; }

  /**
   * std::vector<JustifyChoices>; access: inputOutput. Borrows the stored value.
   * @details The justify field determines horizontal and vertical alignment of
   * text layout, relative to the origin of the object coordinate system.
   */
  const std::vector<JustifyChoices> &getJustify() const { return _justify; }

  /**
   * Access: inputOutput.
   * @details The justify field determines horizontal and vertical alignment of
   * text layout, relative to the origin of the object coordinate system.
   */
  void setJustify(const std::vector<JustifyChoices> &value) {

    _justify = value;
  }

  void setJustify(std::vector<JustifyChoices> &&value) {

    _justify = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Language codes consist of a primary code and a (possibly empty)
   * series of subcodes.
   */
  const SFString &getLanguage() const { return _language; }

  /**
   * Access: inputOutput.
   * @details Language codes consist of a primary code and a (possibly empty)
   * series of subcodes.
   */
  void setLanguage(const SFString &value) { _language = value; }

  void setLanguage(SFString &&value) { _language = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether text direction is left-to-right (true) or right-to-left
   * (false).
   */
  const SFBool &getLeftToRight() const { return _leftToRight; }

  /**
   * Access: inputOutput.
   * @details Whether text direction is left-to-right (true) or right-to-left
   * (false).
   */
  void setLeftToRight(const SFBool &value) { _leftToRight = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details pointSize field specifies the size of text in points.
   */
  const SFFloat &getPointSize() const { return _pointSize; }

  /**
   * Access: inputOutput.
   * @details pointSize field specifies the size of text in points.
   */
  void setPointSize(const SFFloat &value) { _pointSize = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Adjustment factor for line spacing between adjacent lines of text.
   */
  const SFFloat &getSpacing() const { return _spacing; }

  /**
   * Access: inputOutput.
   * @details Adjustment factor for line spacing between adjacent lines of text.
   */
  void setSpacing(const SFFloat &value) {

    validateSpacing(value);

    _spacing = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSpacing() for range validation.
   */
  void setSpacingUnchecked(const SFFloat &value) { _spacing = value; }

  /**
   * FontStyleChoices; access: inputOutput. Borrows the stored value.
   * @details Pick one of four values for text style
   * (PLAIN|BOLD|ITALIC|BOLDITALIC).
   */
  const FontStyleChoices &getStyle() const { return _style; }

  /**
   * Access: inputOutput.
   * @details Pick one of four values for text style
   * (PLAIN|BOLD|ITALIC|BOLDITALIC).
   */
  void setStyle(const FontStyleChoices &value) { _style = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether text direction is top-to-bottom (true) or bottom-to-top
   * (false).
   */
  const SFBool &getTopToBottom() const { return _topToBottom; }

  /**
   * Access: inputOutput.
   * @details Whether text direction is top-to-bottom (true) or bottom-to-top
   * (false).
   */
  void setTopToBottom(const SFBool &value) { _topToBottom = value; }

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
  static void checkRangesSpacing(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

private:
  static void validateSpacing(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("spacing below minimum of 0");
  }

  MFString _family{std::vector<std::string>{"SERIF"}};

  SFBool _horizontal{true};

  std::vector<JustifyChoices> _justify{
      std::vector<JustifyChoices>{JustifyChoices::BEGIN}};

  SFString _language{};

  SFBool _leftToRight{true};

  SFFloat _pointSize{12.0};

  SFFloat _spacing{1.0};

  FontStyleChoices _style{FontStyleChoices::PLAIN};

  SFBool _topToBottom{true};
};

} // namespace x3d::nodes
