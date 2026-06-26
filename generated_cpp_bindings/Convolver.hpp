// Convolver.hpp
#ifndef CONVOLVER_HPP
#define CONVOLVER_HPP

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

#include "X3DSoundProcessingNode.hpp"

/**
 * @class Convolver
 * @brief Convolver performs a linear convolution on a given AudioBuffer, often
 * used to achieve a reverberation effect.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Convolver
 */
class Convolver : public virtual X3DSoundProcessingNode {
public:
  /**
   * @brief Default constructor for Convolver
   */
  Convolver() = default;

  /**
   * @brief Destructor for Convolver
   */
  ~Convolver() = default;

  /**
   * @brief Get the default value for normalize
   * @return SFBool The default value
   */
  static SFBool getDefaultNormalize() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesAudioGraph";
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
   * @brief Gets the value of buffer. AccessType: inputOutput
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   * @return MFFloat The current value of buffer.
   */
  MFFloat getBuffer() const { return _buffer; }

  /**
   * @brief Sets the value of buffer. AccessType: inputOutput
   * @details buffer is a memory-resident audio asset that can contain one or
   * more channels.
   * @param value The new value for buffer.
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
   * @brief Non-validating write of buffer (runtime/reader ingest path).
   * @details Assigns without the range check setBuffer() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBuffer() stays the
   *          enforcement point for programmatic callers.
   */
  void setBufferUnchecked(const MFFloat &value) { _buffer = value; }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: Analyser, AudioClip, AudioDestination,
   * BiquadFilter, BufferAudioSource, ChannelMerger, ChannelSelector,
   * ChannelSplitter, Convolver, Delay, DynamicsCompressor, Gain,
   * ListenerPointSource, MicrophoneSource, MovieTexture, OscillatorSource,
   * Sound, SpatialSound, StreamAudioDestination, StreamAudioSource, WaveShaper
   * @return const std::vector<std::string>& The accepted node type names.
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
   * @brief Sets the value of children. AccessType: inputOutput
   * @details The children field specifies audio-graph sound sources providing
   * input signals for this node.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of normalize. AccessType: inputOutput
   * @details normalize controls whether or not the impulse response from the
   * buffer is scaled by an equal-power normalization when the buffer field is
   * set.
   * @return SFBool The current value of normalize.
   */
  SFBool getNormalize() const { return _normalize; }

  /**
   * @brief Sets the value of normalize. AccessType: inputOutput
   * @details normalize controls whether or not the impulse response from the
   * buffer is scaled by an equal-power normalization when the buffer field is
   * set.
   * @param value The new value for normalize.
   */
  void setNormalize(const SFBool &value) { _normalize = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Convolver").
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
  static void checkRangesBuffer(const MFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateBuffer(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < -1)
        throw std::out_of_range("buffer below minimum of -1");
      if (v > 1)
        throw std::out_of_range("buffer above maximum of 1");
    }
  }

  /**
   * @brief Member variable for buffer.
   */

  MFFloat _buffer{};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for normalize.
   */

  SFBool _normalize{false};
};

#endif // CONVOLVER_HPP