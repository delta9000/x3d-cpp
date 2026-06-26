// TextureProperties.hpp
#ifndef TEXTUREPROPERTIES_HPP
#define TEXTUREPROPERTIES_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

/**
 * @class TextureProperties
 * @brief TextureProperties allows precise fine-grained control over application
 * of image textures to geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#TextureProperties
 */
class TextureProperties : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for TextureProperties
   */
  TextureProperties() = default;

  /**
   * @brief Destructor for TextureProperties
   */
  ~TextureProperties() = default;

  /**
   * @brief Get the default value for anisotropicDegree
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAnisotropicDegree() { return 1; }

  /**
   * @brief Get the default value for borderColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultBorderColor() { return SFColorRGBA{0, 0, 0, 0}; }

  /**
   * @brief Get the default value for borderWidth
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultBorderWidth() { return 0; }

  /**
   * @brief Get the default value for boundaryModeR
   * @return TextureBoundaryModeChoices The default value
   */
  static TextureBoundaryModeChoices getDefaultBoundaryModeR() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  /**
   * @brief Get the default value for boundaryModeS
   * @return TextureBoundaryModeChoices The default value
   */
  static TextureBoundaryModeChoices getDefaultBoundaryModeS() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  /**
   * @brief Get the default value for boundaryModeT
   * @return TextureBoundaryModeChoices The default value
   */
  static TextureBoundaryModeChoices getDefaultBoundaryModeT() {
    return TextureBoundaryModeChoices::REPEAT;
  }

  /**
   * @brief Get the default value for generateMipMaps
   * @return SFBool The default value
   */
  static SFBool getDefaultGenerateMipMaps() { return false; }

  /**
   * @brief Get the default value for magnificationFilter
   * @return TextureMagnificationModeChoices The default value
   */
  static TextureMagnificationModeChoices getDefaultMagnificationFilter() {
    return TextureMagnificationModeChoices::FASTEST;
  }

  /**
   * @brief Get the default value for minificationFilter
   * @return TextureMinificationModeChoices The default value
   */
  static TextureMinificationModeChoices getDefaultMinificationFilter() {
    return TextureMinificationModeChoices::FASTEST;
  }

  /**
   * @brief Get the default value for textureCompression
   * @return TextureCompressionModeChoices The default value
   */
  static TextureCompressionModeChoices getDefaultTextureCompression() {
    return TextureCompressionModeChoices::FASTEST;
  }

  /**
   * @brief Get the default value for texturePriority
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTexturePriority() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "textureProperties"; }

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
   * @brief Gets the value of anisotropicDegree. AccessType: inputOutput
   * @details anisotropicDegree defines minimum degree of anisotropy to account
   * for in texture filtering (1=no effect for symmetric filtering, otherwise
   * provide higher value).
   * @return SFFloat The current value of anisotropicDegree.
   */
  SFFloat getAnisotropicDegree() const { return _anisotropicDegree; }

  /**
   * @brief Sets the value of anisotropicDegree. AccessType: inputOutput
   * @details anisotropicDegree defines minimum degree of anisotropy to account
   * for in texture filtering (1=no effect for symmetric filtering, otherwise
   * provide higher value).
   * @param value The new value for anisotropicDegree.
   */
  void setAnisotropicDegree(const SFFloat &value) {

    validateAnisotropicDegree(value);

    _anisotropicDegree = value;
  }

  /**
   * @brief Non-validating write of anisotropicDegree (runtime/reader ingest
   * path).
   * @details Assigns without the range check setAnisotropicDegree() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAnisotropicDegree() stays the
   *          enforcement point for programmatic callers.
   */
  void setAnisotropicDegreeUnchecked(const SFFloat &value) {
    _anisotropicDegree = value;
  }

  /**
   * @brief Gets the value of borderColor. AccessType: inputOutput
   * @details borderColor defines border pixel color.
   * @return SFColorRGBA The current value of borderColor.
   */
  SFColorRGBA getBorderColor() const { return _borderColor; }

  /**
   * @brief Sets the value of borderColor. AccessType: inputOutput
   * @details borderColor defines border pixel color.
   * @param value The new value for borderColor.
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
   * @brief Non-validating write of borderColor (runtime/reader ingest path).
   * @details Assigns without the range check setBorderColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBorderColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setBorderColorUnchecked(const SFColorRGBA &value) {
    _borderColor = value;
  }

  /**
   * @brief Gets the value of borderWidth. AccessType: inputOutput
   * @details borderWidth number of pixels for texture border.
   * @return SFInt32 The current value of borderWidth.
   */
  SFInt32 getBorderWidth() const { return _borderWidth; }

  /**
   * @brief Sets the value of borderWidth. AccessType: inputOutput
   * @details borderWidth number of pixels for texture border.
   * @param value The new value for borderWidth.
   */
  void setBorderWidth(const SFInt32 &value) {

    validateBorderWidth(value);

    _borderWidth = value;
  }

  /**
   * @brief Non-validating write of borderWidth (runtime/reader ingest path).
   * @details Assigns without the range check setBorderWidth() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBorderWidth() stays the
   *          enforcement point for programmatic callers.
   */
  void setBorderWidthUnchecked(const SFInt32 &value) { _borderWidth = value; }

  /**
   * @brief Gets the value of boundaryModeR. AccessType: inputOutput
   * @details boundaryModeR describes handling of texture-coordinate boundaries.
   * @return TextureBoundaryModeChoices The current value of boundaryModeR.
   */
  TextureBoundaryModeChoices getBoundaryModeR() const { return _boundaryModeR; }

  /**
   * @brief Sets the value of boundaryModeR. AccessType: inputOutput
   * @details boundaryModeR describes handling of texture-coordinate boundaries.
   * @param value The new value for boundaryModeR.
   */
  void setBoundaryModeR(const TextureBoundaryModeChoices &value) {

    _boundaryModeR = value;
  }

  /**
   * @brief Gets the value of boundaryModeS. AccessType: inputOutput
   * @details boundaryModeS describes handling of texture-coordinate boundaries.
   * @return TextureBoundaryModeChoices The current value of boundaryModeS.
   */
  TextureBoundaryModeChoices getBoundaryModeS() const { return _boundaryModeS; }

  /**
   * @brief Sets the value of boundaryModeS. AccessType: inputOutput
   * @details boundaryModeS describes handling of texture-coordinate boundaries.
   * @param value The new value for boundaryModeS.
   */
  void setBoundaryModeS(const TextureBoundaryModeChoices &value) {

    _boundaryModeS = value;
  }

  /**
   * @brief Gets the value of boundaryModeT. AccessType: inputOutput
   * @details boundaryModeT describes handling of texture-coordinate boundaries.
   * @return TextureBoundaryModeChoices The current value of boundaryModeT.
   */
  TextureBoundaryModeChoices getBoundaryModeT() const { return _boundaryModeT; }

  /**
   * @brief Sets the value of boundaryModeT. AccessType: inputOutput
   * @details boundaryModeT describes handling of texture-coordinate boundaries.
   * @param value The new value for boundaryModeT.
   */
  void setBoundaryModeT(const TextureBoundaryModeChoices &value) {

    _boundaryModeT = value;
  }

  /**
   * @brief Gets the value of generateMipMaps. AccessType: initializeOnly
   * @details Determines whether MIPMAPs are generated for texture images.
   * @return SFBool The current value of generateMipMaps.
   */
  SFBool getGenerateMipMaps() const { return _generateMipMaps; }
  /**
   * @brief Data-layer write of generateMipMaps (reader/init ingest path).
   * @details generateMipMaps is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGenerateMipMaps().
   */
  void setGenerateMipMapsUnchecked(const SFBool &value) {
    _generateMipMaps = value;
  }

  /**
   * @brief Gets the value of magnificationFilter. AccessType: inputOutput
   * @details magnificationFilter indicates texture filter when image is smaller
   * than screen space representation.
   * @return TextureMagnificationModeChoices The current value of
   * magnificationFilter.
   */
  TextureMagnificationModeChoices getMagnificationFilter() const {
    return _magnificationFilter;
  }

  /**
   * @brief Sets the value of magnificationFilter. AccessType: inputOutput
   * @details magnificationFilter indicates texture filter when image is smaller
   * than screen space representation.
   * @param value The new value for magnificationFilter.
   */
  void setMagnificationFilter(const TextureMagnificationModeChoices &value) {

    _magnificationFilter = value;
  }

  /**
   * @brief Gets the value of minificationFilter. AccessType: inputOutput
   * @details minificationFilter indicates texture filter when image is larger
   * than screen space representation.
   * @return TextureMinificationModeChoices The current value of
   * minificationFilter.
   */
  TextureMinificationModeChoices getMinificationFilter() const {
    return _minificationFilter;
  }

  /**
   * @brief Sets the value of minificationFilter. AccessType: inputOutput
   * @details minificationFilter indicates texture filter when image is larger
   * than screen space representation.
   * @param value The new value for minificationFilter.
   */
  void setMinificationFilter(const TextureMinificationModeChoices &value) {

    _minificationFilter = value;
  }

  /**
   * @brief Gets the value of textureCompression. AccessType: inputOutput
   * @details textureCompression indicates compression algorithm selection mode.
   * @return TextureCompressionModeChoices The current value of
   * textureCompression.
   */
  TextureCompressionModeChoices getTextureCompression() const {
    return _textureCompression;
  }

  /**
   * @brief Sets the value of textureCompression. AccessType: inputOutput
   * @details textureCompression indicates compression algorithm selection mode.
   * @param value The new value for textureCompression.
   */
  void setTextureCompression(const TextureCompressionModeChoices &value) {

    _textureCompression = value;
  }

  /**
   * @brief Gets the value of texturePriority. AccessType: inputOutput
   * @details texturePriority defines relative priority for this texture when
   * allocating texture memory, an important rendering resource in graphics-card
   * hardware.
   * @return SFFloat The current value of texturePriority.
   */
  SFFloat getTexturePriority() const { return _texturePriority; }

  /**
   * @brief Sets the value of texturePriority. AccessType: inputOutput
   * @details texturePriority defines relative priority for this texture when
   * allocating texture memory, an important rendering resource in graphics-card
   * hardware.
   * @param value The new value for texturePriority.
   */
  void setTexturePriority(const SFFloat &value) {

    validateTexturePriority(value);

    _texturePriority = value;
  }

  /**
   * @brief Non-validating write of texturePriority (runtime/reader ingest
   * path).
   * @details Assigns without the range check setTexturePriority() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setTexturePriority() stays the enforcement point for
   * programmatic callers.
   */
  void setTexturePriorityUnchecked(const SFFloat &value) {
    _texturePriority = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "TextureProperties").
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

    if (value < 1)
      throw std::out_of_range("anisotropicDegree below minimum of 1");
  }

  static void validateBorderColor(const SFColorRGBA &value) {

    if (value.r < 0)
      throw std::out_of_range("borderColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("borderColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("borderColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("borderColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("borderColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("borderColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("borderColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("borderColor.a above maximum of 1");
  }

  static void validateBorderWidth(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("borderWidth below minimum of 0");
  }

  static void validateTexturePriority(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("texturePriority below minimum of 0");
    if (value > 1)
      throw std::out_of_range("texturePriority above maximum of 1");
  }

  /**
   * @brief Member variable for anisotropicDegree.
   */

  SFFloat _anisotropicDegree{1};

  /**
   * @brief Member variable for borderColor.
   */

  SFColorRGBA _borderColor{SFColorRGBA{0, 0, 0, 0}};

  /**
   * @brief Member variable for borderWidth.
   */

  SFInt32 _borderWidth{0};

  /**
   * @brief Member variable for boundaryModeR.
   */

  TextureBoundaryModeChoices _boundaryModeR{TextureBoundaryModeChoices::REPEAT};

  /**
   * @brief Member variable for boundaryModeS.
   */

  TextureBoundaryModeChoices _boundaryModeS{TextureBoundaryModeChoices::REPEAT};

  /**
   * @brief Member variable for boundaryModeT.
   */

  TextureBoundaryModeChoices _boundaryModeT{TextureBoundaryModeChoices::REPEAT};

  /**
   * @brief Member variable for generateMipMaps.
   */

  SFBool _generateMipMaps{false};

  /**
   * @brief Member variable for magnificationFilter.
   */

  TextureMagnificationModeChoices _magnificationFilter{
      TextureMagnificationModeChoices::FASTEST};

  /**
   * @brief Member variable for minificationFilter.
   */

  TextureMinificationModeChoices _minificationFilter{
      TextureMinificationModeChoices::FASTEST};

  /**
   * @brief Member variable for textureCompression.
   */

  TextureCompressionModeChoices _textureCompression{
      TextureCompressionModeChoices::FASTEST};

  /**
   * @brief Member variable for texturePriority.
   */

  SFFloat _texturePriority{0};
};

#endif // TEXTUREPROPERTIES_HPP