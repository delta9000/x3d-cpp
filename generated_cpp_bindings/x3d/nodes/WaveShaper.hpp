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

#include "x3d/nodes/X3DSoundProcessingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class WaveShaper
 * @brief WaveShaper node represents a nonlinear distorter that applies a
 * wave-shaping distortion curve to the signal.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#WaveShaper
 */
class WaveShaper : public virtual X3DSoundProcessingNode {
public:
  WaveShaper() = default;

  ~WaveShaper() = default;

  static WaveShaperOversampleChoices getDefaultOversample() {
    return WaveShaperOversampleChoices::NONE;
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

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

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details The curve field is an array of floating-point numbers describing
   * the distortion to apply.
   */
  const MFFloat &getCurve() const { return _curve; }

  /**
   * Access: inputOutput.
   * @details The curve field is an array of floating-point numbers describing
   * the distortion to apply.
   */
  void setCurve(const MFFloat &value) { _curve = value; }

  void setCurve(MFFloat &&value) { _curve = std::move(value); }

  /**
   * WaveShaperOversampleChoices; access: inputOutput. Borrows the stored value.
   * @details The oversample field is specifies what type of oversampling (if
   * any) should be used when applying the shaping curve.
   */
  const WaveShaperOversampleChoices &getOversample() const {
    return _oversample;
  }

  /**
   * Access: inputOutput.
   * @details The oversample field is specifies what type of oversampling (if
   * any) should be used when applying the shaping curve.
   */
  void setOversample(const WaveShaperOversampleChoices &value) {

    _oversample = value;
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

private:
  MFNode _children{};

  MFFloat _curve{};

  WaveShaperOversampleChoices _oversample{WaveShaperOversampleChoices::NONE};
};

} // namespace x3d::nodes
