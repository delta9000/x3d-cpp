#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Text
 * @brief Text is a 2D (flat) geometry node that can contain multiple lines of
 * string values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/text.html#Text
 */
class Text : public virtual X3DGeometryNode {
public:
  Text() = default;

  ~Text() = default;

  static SFNode getDefaultFontStyle() { return nullptr; }

  static SFFloat getDefaultMaxExtent() { return 0.0; }

  static SFBool getDefaultSolid() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Text"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The fontStyle field can contain a FontStyle or ScreenFontStyle
   * node defining size, family, and style for presented text.
   */
  const SFNode &getFontStyle() const { return _fontStyle; }

  /**
   * @details Permitted X3D node types: X3DFontStyleNode
   */
  static const std::vector<std::string> &acceptableFontStyleNodeTypes() {
    static const std::vector<std::string> types = {"X3DFontStyleNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The fontStyle field can contain a FontStyle or ScreenFontStyle
   * node defining size, family, and style for presented text.
   */
  void setFontStyle(const SFNode &value) { _fontStyle = value; }

  void setFontStyle(SFNode &&value) { _fontStyle = std::move(value); }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Array of length values for each text string in the local
   * coordinate system.
   */
  const MFFloat &getLength() const { return _length; }

  /**
   * Access: inputOutput.
   * @details Array of length values for each text string in the local
   * coordinate system.
   */
  void setLength(const MFFloat &value) {

    validateLength(value);

    _length = value;
  }

  void setLength(MFFloat &&value) {

    validateLength(value);

    _length = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLength() for range validation.
   */
  void setLengthUnchecked(const MFFloat &value) { _length = value; }

  /**
   * MFVec2f; access: outputOnly. Borrows the stored value.
   * @details Array of 2D bounding box values for each line of text in the local
   * coordinate system.
   */
  const MFVec2f &getLineBounds() const { return _lineBounds; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Array of 2D bounding box values for each line of text in the local
   * coordinate system.
   */
  void emitLineBounds(const MFVec2f &value) { _lineBounds = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Limits/compresses all text strings if max string length is longer
   * than maxExtent, as measured in local coordinate system.
   */
  const SFFloat &getMaxExtent() const { return _maxExtent; }

  /**
   * Access: inputOutput.
   * @details Limits/compresses all text strings if max string length is longer
   * than maxExtent, as measured in local coordinate system.
   */
  void setMaxExtent(const SFFloat &value) {

    validateMaxExtent(value);

    _maxExtent = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxExtent() for range validation.
   */
  void setMaxExtentUnchecked(const SFFloat &value) { _maxExtent = value; }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details origin of the text local coordinate system, in units of the
   * coordinate system in which the Text node is embedded.
   */
  const SFVec3f &getOrigin() const { return _origin; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details origin of the text local coordinate system, in units of the
   * coordinate system in which the Text node is embedded.
   */
  void emitOrigin(const SFVec3f &value) { _origin = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Single or multiple string values to present as Text.
   */
  const MFString &getString() const { return _string; }

  /**
   * Access: inputOutput.
   * @details Single or multiple string values to present as Text.
   */
  void setString(const MFString &value) { _string = value; }

  void setString(MFString &&value) { _string = std::move(value); }

  /**
   * SFVec2f; access: outputOnly. Borrows the stored value.
   * @details 2D bounding box value for all lines of text in the local
   * coordinate system.
   */
  const SFVec2f &getTextBounds() const { return _textBounds; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details 2D bounding box value for all lines of text in the local
   * coordinate system.
   */
  void emitTextBounds(const SFVec2f &value) { _textBounds = value; }

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
  static void checkRangesLength(const MFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxExtent(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateLength(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("length below minimum of 0");
    }
  }

  static void validateMaxExtent(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("maxExtent below minimum of 0");
  }

  SFNode _fontStyle{nullptr};

  MFFloat _length{};

  MFVec2f _lineBounds{};

  SFFloat _maxExtent{0.0};

  SFVec3f _origin{};

  SFBool _solid{false};

  MFString _string{};

  SFVec2f _textBounds{};
};

} // namespace x3d::nodes
