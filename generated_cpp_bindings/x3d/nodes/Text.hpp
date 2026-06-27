// Text.hpp
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
  /**
   * @brief Default constructor for Text
   */
  Text() = default;

  /**
   * @brief Destructor for Text
   */
  ~Text() = default;

  /**
   * @brief Get the default value for fontStyle
   * @return SFNode The default value
   */
  static SFNode getDefaultFontStyle() { return nullptr; }

  /**
   * @brief Get the default value for maxExtent
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxExtent() { return 0.0; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Text"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of fontStyle. AccessType: inputOutput
   * @details The fontStyle field can contain a FontStyle or ScreenFontStyle
   * node defining size, family, and style for presented text.
   * @return SFNode The current value of fontStyle.
   */
  SFNode getFontStyle() const { return _fontStyle; }

  /**
   * @brief Acceptable node types for the fontStyle field.
   * @details Permitted X3D node types: X3DFontStyleNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFontStyleNodeTypes() {
    static const std::vector<std::string> types = {"X3DFontStyleNode"};
    return types;
  }

  /**
   * @brief Sets the value of fontStyle. AccessType: inputOutput
   * @details The fontStyle field can contain a FontStyle or ScreenFontStyle
   * node defining size, family, and style for presented text.
   * @param value The new value for fontStyle.
   */
  void setFontStyle(const SFNode &value) { _fontStyle = value; }

  void setFontStyle(SFNode &&value) { _fontStyle = std::move(value); }

  /**
   * @brief Gets the value of length. AccessType: inputOutput
   * @details Array of length values for each text string in the local
   * coordinate system.
   * @return MFFloat The current value of length.
   */
  MFFloat getLength() const { return _length; }

  /**
   * @brief Sets the value of length. AccessType: inputOutput
   * @details Array of length values for each text string in the local
   * coordinate system.
   * @param value The new value for length.
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
   * @brief Non-validating write of length (runtime/reader ingest path).
   * @details Assigns without the range check setLength() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLength() stays the
   *          enforcement point for programmatic callers.
   */
  void setLengthUnchecked(const MFFloat &value) { _length = value; }

  /**
   * @brief Gets the value of lineBounds. AccessType: outputOnly
   * @details Array of 2D bounding box values for each line of text in the local
   * coordinate system.
   * @return MFVec2f The current value of lineBounds.
   */
  MFVec2f getLineBounds() const { return _lineBounds; }

  /**
   * @brief Emit an output value on lineBounds. AccessType: outputOnly
   * @details Array of 2D bounding box values for each line of text in the local
   * coordinate system. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitLineBounds(const MFVec2f &value) { _lineBounds = value; }

  /**
   * @brief Gets the value of maxExtent. AccessType: inputOutput
   * @details Limits/compresses all text strings if max string length is longer
   * than maxExtent, as measured in local coordinate system.
   * @return SFFloat The current value of maxExtent.
   */
  SFFloat getMaxExtent() const { return _maxExtent; }

  /**
   * @brief Sets the value of maxExtent. AccessType: inputOutput
   * @details Limits/compresses all text strings if max string length is longer
   * than maxExtent, as measured in local coordinate system.
   * @param value The new value for maxExtent.
   */
  void setMaxExtent(const SFFloat &value) {

    validateMaxExtent(value);

    _maxExtent = value;
  }

  /**
   * @brief Non-validating write of maxExtent (runtime/reader ingest path).
   * @details Assigns without the range check setMaxExtent() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxExtent() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxExtentUnchecked(const SFFloat &value) { _maxExtent = value; }

  /**
   * @brief Gets the value of origin. AccessType: outputOnly
   * @details origin of the text local coordinate system, in units of the
   * coordinate system in which the Text node is embedded.
   * @return SFVec3f The current value of origin.
   */
  SFVec3f getOrigin() const { return _origin; }

  /**
   * @brief Emit an output value on origin. AccessType: outputOnly
   * @details origin of the text local coordinate system, in units of the
   * coordinate system in which the Text node is embedded. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitOrigin(const SFVec3f &value) { _origin = value; }

  /**
   * @brief Gets the value of solid. AccessType: initializeOnly
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   * @return SFBool The current value of solid.
   */
  SFBool getSolid() const { return _solid; }
  /**
   * @brief Data-layer write of solid (reader/init ingest path).
   * @details solid is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSolid().
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * @brief Gets the value of string. AccessType: inputOutput
   * @details Single or multiple string values to present as Text.
   * @return MFString The current value of string.
   */
  MFString getString() const { return _string; }

  /**
   * @brief Sets the value of string. AccessType: inputOutput
   * @details Single or multiple string values to present as Text.
   * @param value The new value for string.
   */
  void setString(const MFString &value) { _string = value; }

  void setString(MFString &&value) { _string = std::move(value); }

  /**
   * @brief Gets the value of textBounds. AccessType: outputOnly
   * @details 2D bounding box value for all lines of text in the local
   * coordinate system.
   * @return SFVec2f The current value of textBounds.
   */
  SFVec2f getTextBounds() const { return _textBounds; }

  /**
   * @brief Emit an output value on textBounds. AccessType: outputOnly
   * @details 2D bounding box value for all lines of text in the local
   * coordinate system. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTextBounds(const SFVec2f &value) { _textBounds = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Text").
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

      if (v < 0)
        throw std::out_of_range("length below minimum of 0");
    }
  }

  static void validateMaxExtent(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("maxExtent below minimum of 0");
  }

  /**
   * @brief Member variable for fontStyle.
   */

  SFNode _fontStyle{nullptr};

  /**
   * @brief Member variable for length.
   */

  MFFloat _length{};

  /**
   * @brief Member variable for lineBounds.
   */

  MFVec2f _lineBounds{};

  /**
   * @brief Member variable for maxExtent.
   */

  SFFloat _maxExtent{0.0};

  /**
   * @brief Member variable for origin.
   */

  SFVec3f _origin{};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{false};

  /**
   * @brief Member variable for string.
   */

  MFString _string{};

  /**
   * @brief Member variable for textBounds.
   */

  SFVec2f _textBounds{};
};

} // namespace x3d::nodes
