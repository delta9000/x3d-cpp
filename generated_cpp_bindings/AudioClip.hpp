// AudioClip.hpp
#ifndef AUDIOCLIP_HPP
#define AUDIOCLIP_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DTimeDependentNode.hpp"

#include "X3DSoundNode.hpp"

#include "X3DSoundSourceNode.hpp"

#include "X3DUrlObject.hpp"

/**
 * @class AudioClip
 * @brief AudioClip provides audio data used by parent Sound nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#AudioClip
 */
class AudioClip : public virtual X3DSoundSourceNode,
                  public virtual X3DUrlObject {
public:
  /**
   * @brief Default constructor for AudioClip
   */
  AudioClip() = default;

  /**
   * @brief Destructor for AudioClip
   */
  ~AudioClip() = default;

  /**
   * @brief Get the default value for loop
   * @return SFBool The default value
   */
  static SFBool getDefaultLoop() { return false; }

  /**
   * @brief Get the default value for pitch
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPitch() { return 1.0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioClip";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "source"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Sound"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of duration_changed. AccessType: outputOnly
   * @details or -1.
   * @return SFTime The current value of duration_changed.
   */
  SFTime getDuration_changed() const { return _duration_changed; }

  /**
   * @brief Emit an output value on duration_changed. AccessType: outputOnly
   * @details or -1.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitDuration_changed(const SFTime &value) { _duration_changed = value; }

  /**
   * @brief Gets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @return SFBool The current value of loop.
   */
  SFBool getLoop() const { return _loop; }

  /**
   * @brief Sets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @param value The new value for loop.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * @brief Gets the value of pitch. AccessType: inputOutput
   * @details Multiplier for the rate at which sampled sound is played.
   * @return SFFloat The current value of pitch.
   */
  SFFloat getPitch() const { return _pitch; }

  /**
   * @brief Sets the value of pitch. AccessType: inputOutput
   * @details Multiplier for the rate at which sampled sound is played.
   * @param value The new value for pitch.
   */
  void setPitch(const SFFloat &value) { _pitch = value; }

  /**
   * @brief The X3D type name of this node (e.g. "AudioClip").
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

private:
  /**
   * @brief Member variable for duration_changed.
   */

  SFTime _duration_changed{};

  /**
   * @brief Member variable for loop.
   */

  SFBool _loop{false};

  /**
   * @brief Member variable for pitch.
   */

  SFFloat _pitch{1.0};
};

#endif // AUDIOCLIP_HPP