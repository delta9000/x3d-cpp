// MultiTexture.hpp
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

#include "x3d/nodes/X3DTextureNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class MultiTexture
 * @brief MultiTexture applies several individual textures to a single geometry
 * node, enabling a variety of visual effects that include light mapping and
 * environment mapping.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#MultiTexture
 */
class MultiTexture : public virtual X3DTextureNode {
public:
  /**
   * @brief Default constructor for MultiTexture
   */
  MultiTexture() = default;

  /**
   * @brief Destructor for MultiTexture
   */
  ~MultiTexture() = default;

  /**
   * @brief Get the default value for alpha
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAlpha() { return 1; }

  /**
   * @brief Get the default value for color
   * @return SFColor The default value
   */
  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "texture"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of alpha. AccessType: inputOutput
   * @details The alpha field defines the alpha (1-transparency) base value for
   * mode operations.
   * @return SFFloat The current value of alpha.
   */
  SFFloat getAlpha() const { return _alpha; }

  /**
   * @brief Sets the value of alpha. AccessType: inputOutput
   * @details The alpha field defines the alpha (1-transparency) base value for
   * mode operations.
   * @param value The new value for alpha.
   */
  void setAlpha(const SFFloat &value) {

    validateAlpha(value);

    _alpha = value;
  }

  /**
   * @brief Non-validating write of alpha (runtime/reader ingest path).
   * @details Assigns without the range check setAlpha() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAlpha() stays the
   *          enforcement point for programmatic callers.
   */
  void setAlphaUnchecked(const SFFloat &value) { _alpha = value; }

  /**
   * @brief Gets the value of color. AccessType: inputOutput
   * @details The color field defines the RGB base values for mode operations.
   * @return SFColor The current value of color.
   */
  SFColor getColor() const { return _color; }

  /**
   * @brief Sets the value of color. AccessType: inputOutput
   * @details The color field defines the RGB base values for mode operations.
   * @param value The new value for color.
   */
  void setColor(const SFColor &value) {

    validateColor(value);

    _color = value;
  }

  void setColor(SFColor &&value) {

    validateColor(value);

    _color = std::move(value);
  }

  /**
   * @brief Non-validating write of color (runtime/reader ingest path).
   * @details Assigns without the range check setColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setColorUnchecked(const SFColor &value) { _color = value; }

  /**
   * @brief Gets the value of function. AccessType: inputOutput
   * @details function operators COMPLEMENT or ALPHAREPLICATE can be applied
   * after the mode blending operation.
   * @return MFString The current value of function.
   */
  MFString getFunction() const { return _function; }

  /**
   * @brief Sets the value of function. AccessType: inputOutput
   * @details function operators COMPLEMENT or ALPHAREPLICATE can be applied
   * after the mode blending operation.
   * @param value The new value for function.
   */
  void setFunction(const MFString &value) { _function = value; }

  void setFunction(MFString &&value) { _function = std::move(value); }

  /**
   * @brief Gets the value of mode. AccessType: inputOutput
   * @details mode field indicates the type of blending operation, both for
   * color and for alpha channel.
   * @return MFString The current value of mode.
   */
  MFString getMode() const { return _mode; }

  /**
   * @brief Sets the value of mode. AccessType: inputOutput
   * @details mode field indicates the type of blending operation, both for
   * color and for alpha channel.
   * @param value The new value for mode.
   */
  void setMode(const MFString &value) { _mode = value; }

  void setMode(MFString &&value) { _mode = std::move(value); }

  /**
   * @brief Gets the value of source. AccessType: inputOutput
   * @details source field determines whether each image source is treated as
   * DIFFUSE, SPECULAR or a multiplicative FACTOR.
   * @return MFString The current value of source.
   */
  MFString getSource() const { return _source; }

  /**
   * @brief Sets the value of source. AccessType: inputOutput
   * @details source field determines whether each image source is treated as
   * DIFFUSE, SPECULAR or a multiplicative FACTOR.
   * @param value The new value for source.
   */
  void setSource(const MFString &value) { _source = value; }

  void setSource(MFString &&value) { _source = std::move(value); }

  /**
   * @brief Gets the value of texture. AccessType: inputOutput
   * @details Contained texture nodes (ImageTexture, MovieTexture, PixelTexture)
   * that map image(s) to surface geometry, defining each of the different
   * texture channels.
   * @return const MFNode& The current value of texture.
   */
  const MFNode &getTexture() const { return _texture; }

  /**
   * @brief Acceptable node types for the texture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of texture. AccessType: inputOutput
   * @details Contained texture nodes (ImageTexture, MovieTexture, PixelTexture)
   * that map image(s) to surface geometry, defining each of the different
   * texture channels.
   * @param value The new value for texture.
   */
  void setTexture(const MFNode &value) { _texture = value; }

  void setTexture(MFNode &&value) { _texture = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MultiTexture").
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
  static void checkRangesAlpha(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesColor(const SFColor &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateAlpha(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("alpha below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("alpha above maximum of 1");
  }

  static void validateColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("color.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("color.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("color.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("color.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("color.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("color.b above maximum of 1");
  }

  /**
   * @brief Member variable for alpha.
   */

  SFFloat _alpha{1};

  /**
   * @brief Member variable for color.
   */

  SFColor _color{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for function.
   */

  MFString _function{};

  /**
   * @brief Member variable for mode.
   */

  MFString _mode{};

  /**
   * @brief Member variable for source.
   */

  MFString _source{};

  /**
   * @brief Member variable for texture.
   */

  MFNode _texture{};
};

} // namespace x3d::nodes
