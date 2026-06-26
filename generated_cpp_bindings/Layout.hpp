// Layout.hpp
#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DLayoutNode.hpp"

/**
 * @class Layout
 * @brief Layout node is used as layout field of LayoutLayer and LayoutGroup
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#Layout
 */
class Layout : public virtual X3DLayoutNode {
public:
  /**
   * @brief Default constructor for Layout
   */
  Layout() = default;

  /**
   * @brief Destructor for Layout
   */
  ~Layout() = default;

  /**
   * @brief Get the default value for align
   * @return std::vector<LayoutAlignChoices> The default value
   */
  static std::vector<LayoutAlignChoices> getDefaultAlign() {
    return std::vector<LayoutAlignChoices>{};
  }

  /**
   * @brief Get the default value for offset
   * @return MFFloat The default value
   */
  static MFFloat getDefaultOffset() { return std::vector<float>{0, 0}; }

  /**
   * @brief Get the default value for offsetUnits
   * @return std::vector<LayoutUnitsChoices> The default value
   */
  static std::vector<LayoutUnitsChoices> getDefaultOffsetUnits() {
    return std::vector<LayoutUnitsChoices>{};
  }

  /**
   * @brief Get the default value for scaleMode
   * @return std::vector<LayoutScaleModeChoices> The default value
   */
  static std::vector<LayoutScaleModeChoices> getDefaultScaleMode() {
    return std::vector<LayoutScaleModeChoices>{};
  }

  /**
   * @brief Get the default value for size
   * @return MFFloat The default value
   */
  static MFFloat getDefaultSize() { return std::vector<float>{1, 1}; }

  /**
   * @brief Get the default value for sizeUnits
   * @return std::vector<LayoutUnitsChoices> The default value
   */
  static std::vector<LayoutUnitsChoices> getDefaultSizeUnits() {
    return std::vector<LayoutUnitsChoices>{};
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "layout"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Layout"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of align. AccessType: inputOutput
   * @details The align field values align the sized rectangle to an edge or
   * center of the parent rectangle.
   * @return std::vector<LayoutAlignChoices> The current value of align.
   */
  std::vector<LayoutAlignChoices> getAlign() const { return _align; }

  /**
   * @brief Sets the value of align. AccessType: inputOutput
   * @details The align field values align the sized rectangle to an edge or
   * center of the parent rectangle.
   * @param value The new value for align.
   */
  void setAlign(const std::vector<LayoutAlignChoices> &value) {

    _align = value;
  }

  void setAlign(std::vector<LayoutAlignChoices> &&value) {

    _align = std::move(value);
  }

  /**
   * @brief Gets the value of offset. AccessType: inputOutput
   * @details The values of the offset field are used to translate the location
   * of this rectangle after the initial alignment.
   * @return MFFloat The current value of offset.
   */
  MFFloat getOffset() const { return _offset; }

  /**
   * @brief Sets the value of offset. AccessType: inputOutput
   * @details The values of the offset field are used to translate the location
   * of this rectangle after the initial alignment.
   * @param value The new value for offset.
   */
  void setOffset(const MFFloat &value) { _offset = value; }

  void setOffset(MFFloat &&value) { _offset = std::move(value); }

  /**
   * @brief Gets the value of offsetUnits. AccessType: inputOutput
   * @details The offsetUnits field values are used to interprete the offset
   * values.
   * @return std::vector<LayoutUnitsChoices> The current value of offsetUnits.
   */
  std::vector<LayoutUnitsChoices> getOffsetUnits() const {
    return _offsetUnits;
  }

  /**
   * @brief Sets the value of offsetUnits. AccessType: inputOutput
   * @details The offsetUnits field values are used to interprete the offset
   * values.
   * @param value The new value for offsetUnits.
   */
  void setOffsetUnits(const std::vector<LayoutUnitsChoices> &value) {

    _offsetUnits = value;
  }

  void setOffsetUnits(std::vector<LayoutUnitsChoices> &&value) {

    _offsetUnits = std::move(value);
  }

  /**
   * @brief Gets the value of scaleMode. AccessType: inputOutput
   * @details The scaleMode field specifies how the scale of the parent is
   * modified.
   * @return std::vector<LayoutScaleModeChoices> The current value of scaleMode.
   */
  std::vector<LayoutScaleModeChoices> getScaleMode() const {
    return _scaleMode;
  }

  /**
   * @brief Sets the value of scaleMode. AccessType: inputOutput
   * @details The scaleMode field specifies how the scale of the parent is
   * modified.
   * @param value The new value for scaleMode.
   */
  void setScaleMode(const std::vector<LayoutScaleModeChoices> &value) {

    _scaleMode = value;
  }

  void setScaleMode(std::vector<LayoutScaleModeChoices> &&value) {

    _scaleMode = std::move(value);
  }

  /**
   * @brief Gets the value of size. AccessType: inputOutput
   * @details The two values in the size field define the width and height of
   * the layout rectangle.
   * @return MFFloat The current value of size.
   */
  MFFloat getSize() const { return _size; }

  /**
   * @brief Sets the value of size. AccessType: inputOutput
   * @details The two values in the size field define the width and height of
   * the layout rectangle.
   * @param value The new value for size.
   */
  void setSize(const MFFloat &value) { _size = value; }

  void setSize(MFFloat &&value) { _size = std::move(value); }

  /**
   * @brief Gets the value of sizeUnits. AccessType: inputOutput
   * @details The sizeUnits field values are used to interprete the offset
   * values.
   * @return std::vector<LayoutUnitsChoices> The current value of sizeUnits.
   */
  std::vector<LayoutUnitsChoices> getSizeUnits() const { return _sizeUnits; }

  /**
   * @brief Sets the value of sizeUnits. AccessType: inputOutput
   * @details The sizeUnits field values are used to interprete the offset
   * values.
   * @param value The new value for sizeUnits.
   */
  void setSizeUnits(const std::vector<LayoutUnitsChoices> &value) {

    _sizeUnits = value;
  }

  void setSizeUnits(std::vector<LayoutUnitsChoices> &&value) {

    _sizeUnits = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "Layout").
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
   * @brief Member variable for align.
   */

  std::vector<LayoutAlignChoices> _align{std::vector<LayoutAlignChoices>{}};

  /**
   * @brief Member variable for offset.
   */

  MFFloat _offset{std::vector<float>{0, 0}};

  /**
   * @brief Member variable for offsetUnits.
   */

  std::vector<LayoutUnitsChoices> _offsetUnits{
      std::vector<LayoutUnitsChoices>{}};

  /**
   * @brief Member variable for scaleMode.
   */

  std::vector<LayoutScaleModeChoices> _scaleMode{
      std::vector<LayoutScaleModeChoices>{}};

  /**
   * @brief Member variable for size.
   */

  MFFloat _size{std::vector<float>{1, 1}};

  /**
   * @brief Member variable for sizeUnits.
   */

  std::vector<LayoutUnitsChoices> _sizeUnits{std::vector<LayoutUnitsChoices>{}};
};

#endif // LAYOUT_HPP