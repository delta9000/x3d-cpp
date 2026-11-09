#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureProperties
 * @brief TextureProperties allows precise fine-grained control over application
 * of image textures to geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#TextureProperties
 */
class TextureProperties : public virtual X3DNode {
public:
  TextureProperties() = default;

  ~TextureProperties() = default;

  static SFFloat getDefaultAnisotropicDegree() { return 1; }

  static SFColorRGBA getDefaultBorderColor() { return SFColorRGBA{0, 0, 0, 0}; }

  static SFInt32 getDefaultBorderWidth() { return 0; }

  static TextureBoundaryModeChoices getDefaultBoundaryModeR() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  static TextureBoundaryModeChoices getDefaultBoundaryModeS() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  static TextureBoundaryModeChoices getDefaultBoundaryModeT() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  static SFBool getDefaultGenerateMipMaps() { return false; }

  static TextureMagnificationModeChoices getDefaultMagnificationFilter() {
    return TextureMagnificationModeChoices::FASTEST;
  }

  static TextureMinificationModeChoices getDefaultMinificationFilter() {
    return TextureMinificationModeChoices::FASTEST;
  }

  static TextureCompressionModeChoices getDefaultTextureCompression() {
    return TextureCompressionModeChoices::FASTEST;
  }

  static SFFloat getDefaultTexturePriority() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "textureProperties"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details anisotropicDegree defines minimum degree of anisotropy to account
   * for in texture filtering (1=no effect for symmetric filtering, otherwise
   * provide higher value).
   */
  const SFFloat &getAnisotropicDegree() const { return _anisotropicDegree; }

  /**
   * Access: inputOutput.
   * @details anisotropicDegree defines minimum degree of anisotropy to account
   * for in texture filtering (1=no effect for symmetric filtering, otherwise
   * provide higher value).
   */
  void setAnisotropicDegree(const SFFloat &value) {

    validateAnisotropicDegree(value);

    _anisotropicDegree = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAnisotropicDegree() for range validation.
   */
  void setAnisotropicDegreeUnchecked(const SFFloat &value) {
    _anisotropicDegree = value;
  }

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details borderColor defines border pixel color.
   */
  const SFColorRGBA &getBorderColor() const { return _borderColor; }

  /**
   * Access: inputOutput.
   * @details borderColor defines border pixel color.
   */
  void setBorderColor(const SFColorRGBA &value) {

    validateBorderColor(value);

    _borderColor = value;
  }

  void setBorderColor(SFColorRGBA &&value) {

    validateBorderColor(value);

    _borderColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBorderColor() for range validation.
   */
  void setBorderColorUnchecked(const SFColorRGBA &value) {
    _borderColor = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details borderWidth number of pixels for texture border.
   */
  const SFInt32 &getBorderWidth() const { return _borderWidth; }

  /**
   * Access: inputOutput.
   * @details borderWidth number of pixels for texture border.
   */
  void setBorderWidth(const SFInt32 &value) {

    validateBorderWidth(value);

    _borderWidth = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBorderWidth() for range validation.
   */
  void setBorderWidthUnchecked(const SFInt32 &value) { _borderWidth = value; }

  /**
   * TextureBoundaryModeChoices; access: inputOutput. Borrows the stored value.
   * @details boundaryModeR describes handling of texture-coordinate boundaries.
   */
  const TextureBoundaryModeChoices &getBoundaryModeR() const {
    return _boundaryModeR;
  }

  /**
   * Access: inputOutput.
   * @details boundaryModeR describes handling of texture-coordinate boundaries.
   */
  void setBoundaryModeR(const TextureBoundaryModeChoices &value) {

    _boundaryModeR = value;
  }

  /**
   * TextureBoundaryModeChoices; access: inputOutput. Borrows the stored value.
   * @details boundaryModeS describes handling of texture-coordinate boundaries.
   */
  const TextureBoundaryModeChoices &getBoundaryModeS() const {
    return _boundaryModeS;
  }

  /**
   * Access: inputOutput.
   * @details boundaryModeS describes handling of texture-coordinate boundaries.
   */
  void setBoundaryModeS(const TextureBoundaryModeChoices &value) {

    _boundaryModeS = value;
  }

  /**
   * TextureBoundaryModeChoices; access: inputOutput. Borrows the stored value.
   * @details boundaryModeT describes handling of texture-coordinate boundaries.
   */
  const TextureBoundaryModeChoices &getBoundaryModeT() const {
    return _boundaryModeT;
  }

  /**
   * Access: inputOutput.
   * @details boundaryModeT describes handling of texture-coordinate boundaries.
   */
  void setBoundaryModeT(const TextureBoundaryModeChoices &value) {

    _boundaryModeT = value;
  }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Determines whether MIPMAPs are generated for texture images.
   */
  const SFBool &getGenerateMipMaps() const { return _generateMipMaps; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGenerateMipMapsUnchecked(const SFBool &value) {
    _generateMipMaps = value;
  }

  /**
   * TextureMagnificationModeChoices; access: inputOutput. Borrows the stored
   * value.
   * @details magnificationFilter indicates texture filter when image is smaller
   * than screen space representation.
   */
  const TextureMagnificationModeChoices &getMagnificationFilter() const {
    return _magnificationFilter;
  }

  /**
   * Access: inputOutput.
   * @details magnificationFilter indicates texture filter when image is smaller
   * than screen space representation.
   */
  void setMagnificationFilter(const TextureMagnificationModeChoices &value) {

    _magnificationFilter = value;
  }

  /**
   * TextureMinificationModeChoices; access: inputOutput. Borrows the stored
   * value.
   * @details minificationFilter indicates texture filter when image is larger
   * than screen space representation.
   */
  const TextureMinificationModeChoices &getMinificationFilter() const {
    return _minificationFilter;
  }

  /**
   * Access: inputOutput.
   * @details minificationFilter indicates texture filter when image is larger
   * than screen space representation.
   */
  void setMinificationFilter(const TextureMinificationModeChoices &value) {

    _minificationFilter = value;
  }

  /**
   * TextureCompressionModeChoices; access: inputOutput. Borrows the stored
   * value.
   * @details textureCompression indicates compression algorithm selection mode.
   */
  const TextureCompressionModeChoices &getTextureCompression() const {
    return _textureCompression;
  }

  /**
   * Access: inputOutput.
   * @details textureCompression indicates compression algorithm selection mode.
   */
  void setTextureCompression(const TextureCompressionModeChoices &value) {

    _textureCompression = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details texturePriority defines relative priority for this texture when
   * allocating texture memory, an important rendering resource in graphics-card
   * hardware.
   */
  const SFFloat &getTexturePriority() const { return _texturePriority; }

  /**
   * Access: inputOutput.
   * @details texturePriority defines relative priority for this texture when
   * allocating texture memory, an important rendering resource in graphics-card
   * hardware.
   */
  void setTexturePriority(const SFFloat &value) {

    validateTexturePriority(value);

    _texturePriority = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTexturePriority() for range validation.
   */
  void setTexturePriorityUnchecked(const SFFloat &value) {
    _texturePriority = value;
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesAnisotropicDegree(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBorderColor(const SFColorRGBA &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBorderWidth(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTexturePriority(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateAnisotropicDegree(const SFFloat &value) {

    if (value < 1.0f)
      throw std::out_of_range("anisotropicDegree below minimum of 1");
  }

  static void validateBorderColor(const SFColorRGBA &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("borderColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("borderColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("borderColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("borderColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("borderColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("borderColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("borderColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("borderColor.a above maximum of 1");
  }

  static void validateBorderWidth(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("borderWidth below minimum of 0");
  }

  static void validateTexturePriority(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("texturePriority below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("texturePriority above maximum of 1");
  }

  SFFloat _anisotropicDegree{1};

  SFColorRGBA _borderColor{SFColorRGBA{0, 0, 0, 0}};

  SFInt32 _borderWidth{0};

  TextureBoundaryModeChoices _boundaryModeR{TextureBoundaryModeChoices::REPEAT};

  TextureBoundaryModeChoices _boundaryModeS{TextureBoundaryModeChoices::REPEAT};

  TextureBoundaryModeChoices _boundaryModeT{TextureBoundaryModeChoices::REPEAT};

  SFBool _generateMipMaps{false};

  TextureMagnificationModeChoices _magnificationFilter{
      TextureMagnificationModeChoices::FASTEST};

  TextureMinificationModeChoices _minificationFilter{
      TextureMinificationModeChoices::FASTEST};

  TextureCompressionModeChoices _textureCompression{
      TextureCompressionModeChoices::FASTEST};

  SFFloat _texturePriority{0};
};

} // namespace x3d::nodes
