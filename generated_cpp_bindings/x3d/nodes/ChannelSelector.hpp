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

#include "x3d/nodes/X3DSoundNode.hpp"

#include "x3d/nodes/X3DSoundChannelNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ChannelSelector
 * @brief ChannelSelector selects a single channel output from all input
 * channels.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#ChannelSelector
 */
class ChannelSelector : public virtual X3DSoundChannelNode {
public:
  ChannelSelector() = default;

  ~ChannelSelector() = default;

  static SFInt32 getDefaultChannelSelection() { return 0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details channelSelection is single channel of interest from those provided
   * by input nodes.
   */
  const SFInt32 &getChannelSelection() const { return _channelSelection; }

  /**
   * Access: inputOutput.
   * @details channelSelection is single channel of interest from those provided
   * by input nodes.
   */
  void setChannelSelection(const SFInt32 &value) {

    validateChannelSelection(value);

    _channelSelection = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setChannelSelection() for range validation.
   */
  void setChannelSelectionUnchecked(const SFInt32 &value) {
    _channelSelection = value;
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: Analyser, AudioClip, AudioDestination,
   * BiquadFilter, BufferAudioSource, ChannelMerger, ChannelSelector,
   * ChannelSplitter, Convolver, Delay, DynamicsCompressor, Gain,
   * ListenerPointSource, MicrophoneSource, MovieTexture, OscillatorSource,
   * Sound, SpatialSound, StreamAudioDestination, StreamAudioSource, WaveShaper
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"Analyser",
                                                   "AudioClip",
                                                   "AudioDestination",
                                                   "BiquadFilter",
                                                   "BufferAudioSource",
                                                   "ChannelMerger",
                                                   "ChannelSelector",
                                                   "ChannelSplitter",
                                                   "Convolver",
                                                   "Delay",
                                                   "DynamicsCompressor",
                                                   "Gain",
                                                   "ListenerPointSource",
                                                   "MicrophoneSource",
                                                   "MovieTexture",
                                                   "OscillatorSource",
                                                   "Sound",
                                                   "SpatialSound",
                                                   "StreamAudioDestination",
                                                   "StreamAudioSource",
                                                   "WaveShaper"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

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
  static void checkRangesChannelSelection(const SFInt32 &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

private:
  static void validateChannelSelection(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("channelSelection below minimum of 0");
  }

  SFInt32 _channelSelection{0};

  MFNode _children{};
};

} // namespace x3d::nodes
