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

#include "x3d/nodes/X3DLayoutNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Layout
 * @brief Layout node is used as layout field of LayoutLayer and LayoutGroup
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#Layout
 */
class Layout : public virtual X3DLayoutNode {
public:
  Layout() = default;

  ~Layout() = default;

  static std::vector<LayoutAlignChoices> getDefaultAlign() {
    return std::vector<LayoutAlignChoices>{};
  }

  static MFFloat getDefaultOffset() { return std::vector<float>{0, 0}; }

  static std::vector<LayoutUnitsChoices> getDefaultOffsetUnits() {
    return std::vector<LayoutUnitsChoices>{};
  }

  static std::vector<LayoutScaleModeChoices> getDefaultScaleMode() {
    return std::vector<LayoutScaleModeChoices>{};
  }

  static MFFloat getDefaultSize() { return std::vector<float>{1, 1}; }

  static std::vector<LayoutUnitsChoices> getDefaultSizeUnits() {
    return std::vector<LayoutUnitsChoices>{};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "layout"; }

  static std::string componentName() { return "Layout"; }

  static int componentLevel() { return 1; }

  /**
   * std::vector<LayoutAlignChoices>; access: inputOutput. Borrows the stored
   * value.
   * @details The align field values align the sized rectangle to an edge or
   * center of the parent rectangle.
   */
  const std::vector<LayoutAlignChoices> &getAlign() const { return _align; }

  /**
   * Access: inputOutput.
   * @details The align field values align the sized rectangle to an edge or
   * center of the parent rectangle.
   */
  void setAlign(const std::vector<LayoutAlignChoices> &value) {

    _align = value;
  }

  void setAlign(std::vector<LayoutAlignChoices> &&value) {

    _align = std::move(value);
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details The values of the offset field are used to translate the location
   * of this rectangle after the initial alignment.
   */
  const MFFloat &getOffset() const { return _offset; }

  /**
   * Access: inputOutput.
   * @details The values of the offset field are used to translate the location
   * of this rectangle after the initial alignment.
   */
  void setOffset(const MFFloat &value) { _offset = value; }

  void setOffset(MFFloat &&value) { _offset = std::move(value); }

  /**
   * std::vector<LayoutUnitsChoices>; access: inputOutput. Borrows the stored
   * value.
   * @details The offsetUnits field values are used to interprete the offset
   * values.
   */
  const std::vector<LayoutUnitsChoices> &getOffsetUnits() const {
    return _offsetUnits;
  }

  /**
   * Access: inputOutput.
   * @details The offsetUnits field values are used to interprete the offset
   * values.
   */
  void setOffsetUnits(const std::vector<LayoutUnitsChoices> &value) {

    _offsetUnits = value;
  }

  void setOffsetUnits(std::vector<LayoutUnitsChoices> &&value) {

    _offsetUnits = std::move(value);
  }

  /**
   * std::vector<LayoutScaleModeChoices>; access: inputOutput. Borrows the
   * stored value.
   * @details The scaleMode field specifies how the scale of the parent is
   * modified.
   */
  const std::vector<LayoutScaleModeChoices> &getScaleMode() const {
    return _scaleMode;
  }

  /**
   * Access: inputOutput.
   * @details The scaleMode field specifies how the scale of the parent is
   * modified.
   */
  void setScaleMode(const std::vector<LayoutScaleModeChoices> &value) {

    _scaleMode = value;
  }

  void setScaleMode(std::vector<LayoutScaleModeChoices> &&value) {

    _scaleMode = std::move(value);
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details The two values in the size field define the width and height of
   * the layout rectangle.
   */
  const MFFloat &getSize() const { return _size; }

  /**
   * Access: inputOutput.
   * @details The two values in the size field define the width and height of
   * the layout rectangle.
   */
  void setSize(const MFFloat &value) { _size = value; }

  void setSize(MFFloat &&value) { _size = std::move(value); }

  /**
   * std::vector<LayoutUnitsChoices>; access: inputOutput. Borrows the stored
   * value.
   * @details The sizeUnits field values are used to interprete the offset
   * values.
   */
  const std::vector<LayoutUnitsChoices> &getSizeUnits() const {
    return _sizeUnits;
  }

  /**
   * Access: inputOutput.
   * @details The sizeUnits field values are used to interprete the offset
   * values.
   */
  void setSizeUnits(const std::vector<LayoutUnitsChoices> &value) {

    _sizeUnits = value;
  }

  void setSizeUnits(std::vector<LayoutUnitsChoices> &&value) {

    _sizeUnits = std::move(value);
  }

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

private:
  std::vector<LayoutAlignChoices> _align{std::vector<LayoutAlignChoices>{}};

  MFFloat _offset{std::vector<float>{0, 0}};

  std::vector<LayoutUnitsChoices> _offsetUnits{
      std::vector<LayoutUnitsChoices>{}};

  std::vector<LayoutScaleModeChoices> _scaleMode{
      std::vector<LayoutScaleModeChoices>{}};

  MFFloat _size{std::vector<float>{1, 1}};

  std::vector<LayoutUnitsChoices> _sizeUnits{std::vector<LayoutUnitsChoices>{}};
};

} // namespace x3d::nodes
