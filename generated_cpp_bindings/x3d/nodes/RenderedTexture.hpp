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

#include "x3d/nodes/X3DSingleTextureNode.hpp"

#include "x3d/nodes/X3DTexture2DNode.hpp"

#include "x3d/nodes/X3DUrlOutputObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class RenderedTexture
 * @brief RenderedTexture is a texture node that renders the view from a local
 * viewpoint or separate scene into an offscreen buffer, producing an image or
 * depth map that can be rendered from model geometry in real time.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/texturing.html#RenderedTexture
 */
class RenderedTexture : public virtual X3DTexture2DNode,
                        public virtual X3DUrlOutputObject {
public:
  RenderedTexture() = default;

  ~RenderedTexture() = default;

  static SFInt32 getDefaultColorDepth() { return 4; }

  static SFBool getDefaultDepthMap() { return false; }

  static SFInt32 getDefaultHeight() { return 128; }

  static SFInt32 getDefaultMaxNumberFrames() { return 1000; }

  static SFBool getDefaultReplaceImage() { return true; }

  static SFBool getDefaultSingleFrame() { return true; }

  static GeneratedCubeMapTextureUpdateChoices getDefaultUpdate() {
    return GeneratedCubeMapTextureUpdateChoices::NONE;
  }

  static SFTime getDefaultUpdateInterval() { return 0.1; }

  static SFInt32 getDefaultWidth() { return 128; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 4; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details number of color components for the rendered texture.
   */
  const SFInt32 &getColorDepth() const { return _colorDepth; }

  /**
   * Access: inputOutput.
   * @details number of color components for the rendered texture.
   */
  void setColorDepth(const SFInt32 &value) { _colorDepth = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details indicates that a generated texture contains a depth buffer for the
   * image, instead of a color buffer.
   */
  const SFBool &getDepthMap() const { return _depthMap; }

  /**
   * Access: inputOutput.
   * @details indicates that a generated texture contains a depth buffer for the
   * image, instead of a color buffer.
   */
  void setDepthMap(const SFBool &value) { _depthMap = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details image height in pixels.
   */
  const SFInt32 &getHeight() const { return _height; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setHeightUnchecked(const SFInt32 &value) { _height = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details indicates the maximum number of independent frame files (or movie
   * frames) that can be saved for a single series of image captures.
   */
  const SFInt32 &getMaxNumberFrames() const { return _maxNumberFrames; }

  /**
   * Access: inputOutput.
   * @details indicates the maximum number of independent frame files (or movie
   * frames) that can be saved for a single series of image captures.
   */
  void setMaxNumberFrames(const SFInt32 &value) { _maxNumberFrames = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether only a single updated image file or multiple image files
   * can be saved.
   */
  const SFBool &getReplaceImage() const { return _replaceImage; }

  /**
   * Access: inputOutput.
   * @details Whether only a single updated image file or multiple image files
   * can be saved.
   */
  void setReplaceImage(const SFBool &value) { _replaceImage = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details if TRUE only a single image is captured, otherwise continuous
   * updates are captured.
   */
  const SFBool &getSingleFrame() const { return _singleFrame; }

  /**
   * Access: inputOutput.
   * @details if TRUE only a single image is captured, otherwise continuous
   * updates are captured.
   */
  void setSingleFrame(const SFBool &value) { _singleFrame = value; }

  /**
   * GeneratedCubeMapTextureUpdateChoices; access: inputOutput. Borrows the
   * stored value.
   * @details
   */
  const GeneratedCubeMapTextureUpdateChoices &getUpdate() const {
    return _update;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setUpdate(const GeneratedCubeMapTextureUpdateChoices &value) {

    _update = value;
  }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details indicates time intervals between render captures when the update
   * field is "ALWAYS".
   */
  const SFTime &getUpdateInterval() const { return _updateInterval; }

  /**
   * Access: inputOutput.
   * @details indicates time intervals between render captures when the update
   * field is "ALWAYS".
   */
  void setUpdateInterval(const SFTime &value) { _updateInterval = value; }

  void setUpdateInterval(SFTime &&value) { _updateInterval = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details image height in pixels.
   */
  const SFInt32 &getWidth() const { return _width; }

  /**
   * Access: inputOutput.
   * @details image height in pixels.
   */
  void setWidth(const SFInt32 &value) { _width = value; }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  SFInt32 _colorDepth{4};

  SFBool _depthMap{false};

  SFInt32 _height{128};

  SFInt32 _maxNumberFrames{1000};

  SFBool _replaceImage{true};

  SFBool _singleFrame{true};

  GeneratedCubeMapTextureUpdateChoices _update{
      GeneratedCubeMapTextureUpdateChoices::NONE};

  SFTime _updateInterval{0.1};

  SFInt32 _width{128};
};

} // namespace x3d::nodes
