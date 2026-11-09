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
  MultiTexture() = default;

  ~MultiTexture() = default;

  static SFFloat getDefaultAlpha() { return 1; }

  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The alpha field defines the alpha (1-transparency) base value for
   * mode operations.
   */
  const SFFloat &getAlpha() const { return _alpha; }

  /**
   * Access: inputOutput.
   * @details The alpha field defines the alpha (1-transparency) base value for
   * mode operations.
   */
  void setAlpha(const SFFloat &value) {

    validateAlpha(value);

    _alpha = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAlpha() for range validation.
   */
  void setAlphaUnchecked(const SFFloat &value) { _alpha = value; }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details The color field defines the RGB base values for mode operations.
   */
  const SFColor &getColor() const { return _color; }

  /**
   * Access: inputOutput.
   * @details The color field defines the RGB base values for mode operations.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setColor() for range validation.
   */
  void setColorUnchecked(const SFColor &value) { _color = value; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details function operators COMPLEMENT or ALPHAREPLICATE can be applied
   * after the mode blending operation.
   */
  const MFString &getFunction() const { return _function; }

  /**
   * Access: inputOutput.
   * @details function operators COMPLEMENT or ALPHAREPLICATE can be applied
   * after the mode blending operation.
   */
  void setFunction(const MFString &value) { _function = value; }

  void setFunction(MFString &&value) { _function = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details mode field indicates the type of blending operation, both for
   * color and for alpha channel.
   */
  const MFString &getMode() const { return _mode; }

  /**
   * Access: inputOutput.
   * @details mode field indicates the type of blending operation, both for
   * color and for alpha channel.
   */
  void setMode(const MFString &value) { _mode = value; }

  void setMode(MFString &&value) { _mode = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details source field determines whether each image source is treated as
   * DIFFUSE, SPECULAR or a multiplicative FACTOR.
   */
  const MFString &getSource() const { return _source; }

  /**
   * Access: inputOutput.
   * @details source field determines whether each image source is treated as
   * DIFFUSE, SPECULAR or a multiplicative FACTOR.
   */
  void setSource(const MFString &value) { _source = value; }

  void setSource(MFString &&value) { _source = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Contained texture nodes (ImageTexture, MovieTexture, PixelTexture)
   * that map image(s) to surface geometry, defining each of the different
   * texture channels.
   */
  const MFNode &getTexture() const { return _texture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Contained texture nodes (ImageTexture, MovieTexture, PixelTexture)
   * that map image(s) to surface geometry, defining each of the different
   * texture channels.
   */
  void setTexture(const MFNode &value) { _texture = value; }

  void setTexture(MFNode &&value) { _texture = std::move(value); }

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

  SFFloat _alpha{1};

  SFColor _color{SFColor{1, 1, 1}};

  MFString _function{};

  MFString _mode{};

  MFString _source{};

  MFNode _texture{};
};

} // namespace x3d::nodes
