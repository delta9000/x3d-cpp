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

#include "x3d/nodes/X3DTimeDependentNode.hpp"

#include "x3d/nodes/X3DSoundNode.hpp"

#include "x3d/nodes/X3DSoundSourceNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DTextureNode.hpp"

#include "x3d/nodes/X3DSingleTextureNode.hpp"

#include "x3d/nodes/X3DTexture2DNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class MovieTexture
 * @brief MovieTexture applies a 2D movie image to surface geometry, or provides
 * audio for a Sound node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/texturing.html#MovieTexture
 */
class MovieTexture : public virtual X3DSoundSourceNode,
                     public virtual X3DTexture2DNode,
                     public virtual X3DUrlObject {
public:
  MovieTexture() = default;

  ~MovieTexture() = default;

  static SFBool getDefaultLoop() { return false; }

  static SFFloat getDefaultPitch() { return 1.0; }

  static SFFloat getDefaultSpeed() { return 1.0; }

  static SFNode getDefaultTextureProperties() { return nullptr; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3dUrlObjectTexture";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 3; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getColorDepth() const { return _colorDepth; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitColorDepth(const SFInt32 &value) { _colorDepth = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details or -1.
   */
  const SFTime &getDuration_changed() const { return _duration_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details or -1.
   */
  void emitDuration_changed(const SFTime &value) { _duration_changed = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getHasSound() const { return _hasSound; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHasSound(const SFInt32 &value) { _hasSound = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getHeight() const { return _height; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHeight(const SFInt32 &value) { _height = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  const SFBool &getLoop() const { return _loop; }

  /**
   * Access: inputOutput.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Multiplier for the rate at which sampled sound is played.
   */
  const SFFloat &getPitch() const { return _pitch; }

  /**
   * Access: inputOutput.
   * @details Multiplier for the rate at which sampled sound is played.
   */
  void setPitch(const SFFloat &value) { _pitch = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Factor for how fast the movie (or soundtrack) is played.
   */
  const SFFloat &getSpeed() const { return _speed; }

  /**
   * Access: inputOutput.
   * @details Factor for how fast the movie (or soundtrack) is played.
   */
  void setSpeed(const SFFloat &value) { _speed = value; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details Optional single contained TextureProperties node that can specify
   * additional visual attributes applied to corresponding texture images.
   */
  const SFNode &getTextureProperties() const { return _textureProperties; }

  /**
   * @details Permitted X3D node types: TextureProperties
   */
  static const std::vector<std::string> &
  acceptableTexturePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"TextureProperties"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexturePropertiesUnchecked(const SFNode &value) {
    _textureProperties = value;
  }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getWidth() const { return _width; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitWidth(const SFInt32 &value) { _width = value; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFInt32 _colorDepth{};

  SFTime _duration_changed{};

  SFInt32 _hasSound{};

  SFInt32 _height{};

  SFBool _loop{false};

  SFFloat _pitch{1.0};

  SFFloat _speed{1.0};

  SFNode _textureProperties{nullptr};

  SFInt32 _width{};
};

} // namespace x3d::nodes
