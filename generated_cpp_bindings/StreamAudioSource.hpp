// StreamAudioSource.hpp
#ifndef STREAMAUDIOSOURCE_HPP
#define STREAMAUDIOSOURCE_HPP

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

/**
 * @class StreamAudioSource
 * @brief StreamAudioSource operates as an audio source whose media is received
 * from a MediaStream obtained using the WebRTC or Media Capture and Streams
 * APIs.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#StreamAudioSource
 */
class StreamAudioSource : public virtual X3DSoundSourceNode {
public:
  /**
   * @brief Default constructor for StreamAudioSource
   */
  StreamAudioSource() = default;

  /**
   * @brief Destructor for StreamAudioSource
   */
  ~StreamAudioSource() = default;

  /**
   * @brief Get the default value for channelCountMode
   * @return ChannelCountModeChoices The default value
   */
  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  /**
   * @brief Get the default value for channelInterpretation
   * @return ChannelInterpretationChoices The default value
   */
  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Sound"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of channelCountMode. AccessType: inputOutput
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   * @return ChannelCountModeChoices The current value of channelCountMode.
   */
  ChannelCountModeChoices getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * @brief Sets the value of channelCountMode. AccessType: inputOutput
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   * @param value The new value for channelCountMode.
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * @brief Gets the value of channelInterpretation. AccessType: inputOutput
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   * @return ChannelInterpretationChoices The current value of
   * channelInterpretation.
   */
  ChannelInterpretationChoices getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * @brief Sets the value of channelInterpretation. AccessType: inputOutput
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   * @param value The new value for channelInterpretation.
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

  /**
   * @brief Gets the value of streamIdentifier. AccessType: inputOutput
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   * @return SFString The current value of streamIdentifier.
   */
  SFString getStreamIdentifier() const { return _streamIdentifier; }

  /**
   * @brief Sets the value of streamIdentifier. AccessType: inputOutput
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   * @param value The new value for streamIdentifier.
   */
  void setStreamIdentifier(const SFString &value) { _streamIdentifier = value; }

  void setStreamIdentifier(SFString &&value) {

    _streamIdentifier = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "StreamAudioSource").
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

private:
  /**
   * @brief Member variable for channelCountMode.
   */

  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  /**
   * @brief Member variable for channelInterpretation.
   */

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  /**
   * @brief Member variable for streamIdentifier.
   */

  SFString _streamIdentifier{};
};

#endif // STREAMAUDIOSOURCE_HPP