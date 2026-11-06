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

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class AudioClip
 * @brief AudioClip provides audio data used by parent Sound nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#AudioClip
 */
class AudioClip : public virtual X3DSoundSourceNode,
                  public virtual X3DUrlObject {
public:
  AudioClip() = default;

  ~AudioClip() = default;

  static SFBool getDefaultLoop() { return false; }

  static SFFloat getDefaultPitch() { return 1.0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioClip";
  }

  static std::string getDefaultContainerField() { return "source"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 1; }

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

private:
  SFTime _duration_changed{};

  SFBool _loop{false};

  SFFloat _pitch{1.0};
};

} // namespace x3d::nodes
