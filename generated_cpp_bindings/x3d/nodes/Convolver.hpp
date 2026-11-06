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
 * @class Convolver
 * @brief Convolver performs a linear convolution on a given AudioBuffer, often
 * used to achieve a reverberation effect.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Convolver
 */
class Convolver : public virtual X3DSoundProcessingNode {
public:
  Convolver() = default;

  ~Convolver() = default;

  static SFBool getDefaultNormalize() { return false; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   */
  const MFFloat &getBuffer() const { return _buffer; }

  /**
   * Access: inputOutput.
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   */
  void setBuffer(const MFFloat &value) {

    validateBuffer(value);

    _buffer = value;
  }

  void setBuffer(MFFloat &&value) {

    validateBuffer(value);

    _buffer = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBuffer() for range validation.
   */
  void setBufferUnchecked(const MFFloat &value) { _buffer = value; }

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
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details normalize controls whether or not the impulse response from the
   * buffer is scaled by an equal-power normalization when the buffer field is
   * set.
   */
  const SFBool &getNormalize() const { return _normalize; }

  /**
   * Access: inputOutput.
   * @details normalize controls whether or not the impulse response from the
   * buffer is scaled by an equal-power normalization when the buffer field is
   * set.
   */
  void setNormalize(const SFBool &value) { _normalize = value; }

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
  static void checkRangesBuffer(const MFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateBuffer(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < -1.0f)
        throw std::out_of_range("buffer below minimum of -1");
      if (v > 1.0f)
        throw std::out_of_range("buffer above maximum of 1");
    }
  }

  MFFloat _buffer{};

  MFNode _children{};

  SFBool _normalize{false};
};

} // namespace x3d::nodes
