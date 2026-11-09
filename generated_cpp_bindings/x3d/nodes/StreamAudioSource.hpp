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

namespace x3d::nodes {
using namespace x3d::core;

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
  StreamAudioSource() = default;

  ~StreamAudioSource() = default;

  static ChannelCountModeChoices getDefaultChannelCountMode() {
    return ChannelCountModeChoices::MAX;
  }

  static ChannelInterpretationChoices getDefaultChannelInterpretation() {
    return ChannelInterpretationChoices::SPEAKERS;
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * ChannelCountModeChoices; access: inputOutput. Borrows the stored value.
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   */
  const ChannelCountModeChoices &getChannelCountMode() const {
    return _channelCountMode;
  }

  /**
   * Access: inputOutput.
   * @details channelCountMode determines how individual channels are counted
   * when up-mixing and down-mixing connections to any inputs.
   */
  void setChannelCountMode(const ChannelCountModeChoices &value) {

    _channelCountMode = value;
  }

  /**
   * ChannelInterpretationChoices; access: inputOutput. Borrows the stored
   * value.
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   */
  const ChannelInterpretationChoices &getChannelInterpretation() const {
    return _channelInterpretation;
  }

  /**
   * Access: inputOutput.
   * @details channelInterpretation determines how individual channels are
   * treated when up-mixing and down-mixing connections to any inputs.
   */
  void setChannelInterpretation(const ChannelInterpretationChoices &value) {

    _channelInterpretation = value;
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   */
  const SFString &getStreamIdentifier() const { return _streamIdentifier; }

  /**
   * Access: inputOutput.
   * @details Stream identification TBD Hint: W3C Media Capture and Streams
   * https://www.
   */
  void setStreamIdentifier(const SFString &value) { _streamIdentifier = value; }

  void setStreamIdentifier(SFString &&value) {

    _streamIdentifier = std::move(value);
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

private:
  ChannelCountModeChoices _channelCountMode{ChannelCountModeChoices::MAX};

  ChannelInterpretationChoices _channelInterpretation{
      ChannelInterpretationChoices::SPEAKERS};

  SFString _streamIdentifier{};
};

} // namespace x3d::nodes
