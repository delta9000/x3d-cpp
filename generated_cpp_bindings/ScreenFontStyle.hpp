// ScreenFontStyle.hpp
#ifndef SCREENFONTSTYLE_HPP
#define SCREENFONTSTYLE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DFontStyleNode.hpp"

/**
 * @class ScreenFontStyle
 * @brief ScreenFontStyle is an X3DFontStyleNode defines the size, family,
 * justification, and other styles used within a screen layout.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#ScreenFontStyle
 */
class ScreenFontStyle : public virtual X3DFontStyleNode {
public:
  /**
   * @brief Default constructor for ScreenFontStyle
   */
  ScreenFontStyle() = default;

  /**
   * @brief Destructor for ScreenFontStyle
   */
  ~ScreenFontStyle() = default;

  /**
   * @brief Get the default value for family
   * @return MFString The default value
   */
  static MFString getDefaultFamily() {
    return std::vector<std::string>{"SERIF"};
  }

  /**
   * @brief Get the default value for horizontal
   * @return SFBool The default value
   */
  static SFBool getDefaultHorizontal() { return true; }

  /**
   * @brief Get the default value for justify
   * @return std::vector<JustifyChoices> The default value
   */
  static std::vector<JustifyChoices> getDefaultJustify() {
    return std::vector<JustifyChoices>{JustifyChoices::BEGIN};
  }

  /**
   * @brief Get the default value for leftToRight
   * @return SFBool The default value
   */
  static SFBool getDefaultLeftToRight() { return true; }

  /**
   * @brief Get the default value for pointSize
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPointSize() { return 12.0; }

  /**
   * @brief Get the default value for spacing
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpacing() { return 1.0; }

  /**
   * @brief Get the default value for style
   * @return FontStyleChoices The default value
   */
  static FontStyleChoices getDefaultStyle() { return FontStyleChoices::PLAIN; }

  /**
   * @brief Get the default value for topToBottom
   * @return SFBool The default value
   */
  static SFBool getDefaultTopToBottom() { return true; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "fontStyle"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Layout"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of family. AccessType: inputOutput
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   * @return MFString The current value of family.
   */
  MFString getFamily() const { return _family; }

  /**
   * @brief Sets the value of family. AccessType: inputOutput
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   * @param value The new value for family.
   */
  void setFamily(const MFString &value) { _family = value; }

  void setFamily(MFString &&value) { _family = std::move(value); }

  /**
   * @brief Gets the value of horizontal. AccessType: inputOutput
   * @details Whether text direction is horizontal (true) or vertical (false).
   * @return SFBool The current value of horizontal.
   */
  SFBool getHorizontal() const { return _horizontal; }

  /**
   * @brief Sets the value of horizontal. AccessType: inputOutput
   * @details Whether text direction is horizontal (true) or vertical (false).
   * @param value The new value for horizontal.
   */
  void setHorizontal(const SFBool &value) { _horizontal = value; }

  /**
   * @brief Gets the value of justify. AccessType: inputOutput
   * @details The justify field determines horizontal and vertical alignment of
   * text layout, relative to the origin of the object coordinate system.
   * @return std::vector<JustifyChoices> The current value of justify.
   */
  std::vector<JustifyChoices> getJustify() const { return _justify; }

  /**
   * @brief Sets the value of justify. AccessType: inputOutput
   * @details The justify field determines horizontal and vertical alignment of
   * text layout, relative to the origin of the object coordinate system.
   * @param value The new value for justify.
   */
  void setJustify(const std::vector<JustifyChoices> &value) {

    _justify = value;
  }

  void setJustify(std::vector<JustifyChoices> &&value) {

    _justify = std::move(value);
  }

  /**
   * @brief Gets the value of language. AccessType: inputOutput
   * @details Language codes consist of a primary code and a (possibly empty)
   * series of subcodes.
   * @return SFString The current value of language.
   */
  SFString getLanguage() const { return _language; }

  /**
   * @brief Sets the value of language. AccessType: inputOutput
   * @details Language codes consist of a primary code and a (possibly empty)
   * series of subcodes.
   * @param value The new value for language.
   */
  void setLanguage(const SFString &value) { _language = value; }

  void setLanguage(SFString &&value) { _language = std::move(value); }

  /**
   * @brief Gets the value of leftToRight. AccessType: inputOutput
   * @details Whether text direction is left-to-right (true) or right-to-left
   * (false).
   * @return SFBool The current value of leftToRight.
   */
  SFBool getLeftToRight() const { return _leftToRight; }

  /**
   * @brief Sets the value of leftToRight. AccessType: inputOutput
   * @details Whether text direction is left-to-right (true) or right-to-left
   * (false).
   * @param value The new value for leftToRight.
   */
  void setLeftToRight(const SFBool &value) { _leftToRight = value; }

  /**
   * @brief Gets the value of pointSize. AccessType: inputOutput
   * @details pointSize field specifies the size of text in points.
   * @return SFFloat The current value of pointSize.
   */
  SFFloat getPointSize() const { return _pointSize; }

  /**
   * @brief Sets the value of pointSize. AccessType: inputOutput
   * @details pointSize field specifies the size of text in points.
   * @param value The new value for pointSize.
   */
  void setPointSize(const SFFloat &value) { _pointSize = value; }

  /**
   * @brief Gets the value of spacing. AccessType: inputOutput
   * @details Adjustment factor for line spacing between adjacent lines of text.
   * @return SFFloat The current value of spacing.
   */
  SFFloat getSpacing() const { return _spacing; }

  /**
   * @brief Sets the value of spacing. AccessType: inputOutput
   * @details Adjustment factor for line spacing between adjacent lines of text.
   * @param value The new value for spacing.
   */
  void setSpacing(const SFFloat &value) {

    validateSpacing(value);

    _spacing = value;
  }

  /**
   * @brief Non-validating write of spacing (runtime/reader ingest path).
   * @details Assigns without the range check setSpacing() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSpacing() stays the
   *          enforcement point for programmatic callers.
   */
  void setSpacingUnchecked(const SFFloat &value) { _spacing = value; }

  /**
   * @brief Gets the value of style. AccessType: inputOutput
   * @details Pick one of four values for text style
   * (PLAIN|BOLD|ITALIC|BOLDITALIC).
   * @return FontStyleChoices The current value of style.
   */
  FontStyleChoices getStyle() const { return _style; }

  /**
   * @brief Sets the value of style. AccessType: inputOutput
   * @details Pick one of four values for text style
   * (PLAIN|BOLD|ITALIC|BOLDITALIC).
   * @param value The new value for style.
   */
  void setStyle(const FontStyleChoices &value) { _style = value; }

  /**
   * @brief Gets the value of topToBottom. AccessType: inputOutput
   * @details Whether text direction is top-to-bottom (true) or bottom-to-top
   * (false).
   * @return SFBool The current value of topToBottom.
   */
  SFBool getTopToBottom() const { return _topToBottom; }

  /**
   * @brief Sets the value of topToBottom. AccessType: inputOutput
   * @details Whether text direction is top-to-bottom (true) or bottom-to-top
   * (false).
   * @param value The new value for topToBottom.
   */
  void setTopToBottom(const SFBool &value) { _topToBottom = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ScreenFontStyle").
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
  static void checkRangesSpacing(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

private:
  static void validateSpacing(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("spacing below minimum of 0");
  }

  /**
   * @brief Member variable for family.
   */

  MFString _family{std::vector<std::string>{"SERIF"}};

  /**
   * @brief Member variable for horizontal.
   */

  SFBool _horizontal{true};

  /**
   * @brief Member variable for justify.
   */

  std::vector<JustifyChoices> _justify{
      std::vector<JustifyChoices>{JustifyChoices::BEGIN}};

  /**
   * @brief Member variable for language.
   */

  SFString _language{};

  /**
   * @brief Member variable for leftToRight.
   */

  SFBool _leftToRight{true};

  /**
   * @brief Member variable for pointSize.
   */

  SFFloat _pointSize{12.0};

  /**
   * @brief Member variable for spacing.
   */

  SFFloat _spacing{1.0};

  /**
   * @brief Member variable for style.
   */

  FontStyleChoices _style{FontStyleChoices::PLAIN};

  /**
   * @brief Member variable for topToBottom.
   */

  SFBool _topToBottom{true};
};

#endif // SCREENFONTSTYLE_HPP