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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SpatialSound
 * @brief The SpatialSound node controls the 3D spatialization of sound playback
 * by a child AudioClip or MovieTexture node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Sound
 */
class SpatialSound : public virtual X3DSoundNode {
public:
  SpatialSound() = default;

  ~SpatialSound() = default;

  static SFFloat getDefaultConeInnerAngle() { return 6.2832; }

  static SFFloat getDefaultConeOuterAngle() { return 6.2832; }

  static SFFloat getDefaultConeOuterGain() { return 0; }

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

  static DistanceModelChoices getDefaultDistanceModel() {
    return DistanceModelChoices::INVERSE;
  }

  static SFBool getDefaultDopplerEnabled() { return false; }

  static SFBool getDefaultEnableHRTF() { return false; }

  static SFFloat getDefaultGain() { return 1; }

  static SFFloat getDefaultIntensity() { return 1; }

  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultMaxDistance() { return 10000; }

  static SFFloat getDefaultPriority() { return 0; }

  static SFFloat getDefaultReferenceDistance() { return 1; }

  static SFFloat getDefaultRolloffFactor() { return 1; }

  static SFBool getDefaultSpatialize() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details coneInnerAngle is centered along direction and defines the inner
   * conical volume, inside of which no source gain reduction occurs.
   */
  const SFFloat &getConeInnerAngle() const { return _coneInnerAngle; }

  /**
   * Access: inputOutput.
   * @details coneInnerAngle is centered along direction and defines the inner
   * conical volume, inside of which no source gain reduction occurs.
   */
  void setConeInnerAngle(const SFFloat &value) {

    validateConeInnerAngle(value);

    _coneInnerAngle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setConeInnerAngle() for range validation.
   */
  void setConeInnerAngleUnchecked(const SFFloat &value) {
    _coneInnerAngle = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details coneOuterAngle is centered along direction and defines an outer
   * conical volume, within which the sound gain decreases linearly from full
   * gain to coneOuterGain.
   */
  const SFFloat &getConeOuterAngle() const { return _coneOuterAngle; }

  /**
   * Access: inputOutput.
   * @details coneOuterAngle is centered along direction and defines an outer
   * conical volume, within which the sound gain decreases linearly from full
   * gain to coneOuterGain.
   */
  void setConeOuterAngle(const SFFloat &value) {

    validateConeOuterAngle(value);

    _coneOuterAngle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setConeOuterAngle() for range validation.
   */
  void setConeOuterAngleUnchecked(const SFFloat &value) {
    _coneOuterAngle = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details coneOuterGain is minimum gain value found outside coneOuterAngle.
   */
  const SFFloat &getConeOuterGain() const { return _coneOuterGain; }

  /**
   * Access: inputOutput.
   * @details coneOuterGain is minimum gain value found outside coneOuterAngle.
   */
  void setConeOuterGain(const SFFloat &value) { _coneOuterGain = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details direction of sound axis, relative to local coordinate system.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details direction of sound axis, relative to local coordinate system.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * DistanceModelChoices; access: inputOutput. Borrows the stored value.
   * @details distanceModel determines how field specifies which algorithm to
   * use for sound attenuation, corresponding to distance between an audio
   * source and a listener, as it moves away from the listener.
   */
  const DistanceModelChoices &getDistanceModel() const {
    return _distanceModel;
  }

  /**
   * Access: inputOutput.
   * @details distanceModel determines how field specifies which algorithm to
   * use for sound attenuation, corresponding to distance between an audio
   * source and a listener, as it moves away from the listener.
   */
  void setDistanceModel(const DistanceModelChoices &value) {

    _distanceModel = value;
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details dopplerEnabled enables/disables whether real-time Doppler effects
   * (due to relation motion between sources and listeners) are computed by
   * browser between virtual sound sources and active listening locations, then
   * applied to received frequency at active listening locations.
   */
  const SFBool &getDopplerEnabled() const { return _dopplerEnabled; }

  /**
   * Access: inputOutput.
   * @details dopplerEnabled enables/disables whether real-time Doppler effects
   * (due to relation motion between sources and listeners) are computed by
   * browser between virtual sound sources and active listening locations, then
   * applied to received frequency at active listening locations.
   */
  void setDopplerEnabled(const SFBool &value) { _dopplerEnabled = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details enableHRTF enables/disables Head Related Transfer Function (HRTF)
   * auralization, if available.
   */
  const SFBool &getEnableHRTF() const { return _enableHRTF; }

  /**
   * Access: inputOutput.
   * @details enableHRTF enables/disables Head Related Transfer Function (HRTF)
   * auralization, if available.
   */
  void setEnableHRTF(const SFBool &value) { _enableHRTF = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The gain field is a factor that represents the amount of linear
   * amplification to apply to the output of the node.
   */
  const SFFloat &getGain() const { return _gain; }

  /**
   * Access: inputOutput.
   * @details The gain field is a factor that represents the amount of linear
   * amplification to apply to the output of the node.
   */
  void setGain(const SFFloat &value) { _gain = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Factor [0,1] adjusting loudness (decibels) of emitted sound.
   */
  const SFFloat &getIntensity() const { return _intensity; }

  /**
   * Access: inputOutput.
   * @details Factor [0,1] adjusting loudness (decibels) of emitted sound.
   */
  void setIntensity(const SFFloat &value) {

    validateIntensity(value);

    _intensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setIntensity() for range validation.
   */
  void setIntensityUnchecked(const SFFloat &value) { _intensity = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position of sound ellipsoid center, relative to local coordinate
   * system.
   */
  const SFVec3f &getLocation() const { return _location; }

  /**
   * Access: inputOutput.
   * @details Position of sound ellipsoid center, relative to local coordinate
   * system.
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxDistance is the maximum distance where sound is renderable
   * between source and listener, after which no reduction in sound volume
   * occurs.
   */
  const SFFloat &getMaxDistance() const { return _maxDistance; }

  /**
   * Access: inputOutput.
   * @details maxDistance is the maximum distance where sound is renderable
   * between source and listener, after which no reduction in sound volume
   * occurs.
   */
  void setMaxDistance(const SFFloat &value) {

    validateMaxDistance(value);

    _maxDistance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxDistance() for range validation.
   */
  void setMaxDistanceUnchecked(const SFFloat &value) { _maxDistance = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Player hint [0,1] if needed to choose which sounds to play.
   */
  const SFFloat &getPriority() const { return _priority; }

  /**
   * Access: inputOutput.
   * @details Player hint [0,1] if needed to choose which sounds to play.
   */
  void setPriority(const SFFloat &value) {

    validatePriority(value);

    _priority = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setPriority() for range validation.
   */
  void setPriorityUnchecked(const SFFloat &value) { _priority = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details referenceDistance for reducing volume as source moves further from
   * the listener.
   */
  const SFFloat &getReferenceDistance() const { return _referenceDistance; }

  /**
   * Access: inputOutput.
   * @details referenceDistance for reducing volume as source moves further from
   * the listener.
   */
  void setReferenceDistance(const SFFloat &value) {

    validateReferenceDistance(value);

    _referenceDistance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setReferenceDistance() for range validation.
   */
  void setReferenceDistanceUnchecked(const SFFloat &value) {
    _referenceDistance = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details rolloffFactor indicates how quickly volume is reduced as source
   * moves further from listener.
   */
  const SFFloat &getRolloffFactor() const { return _rolloffFactor; }

  /**
   * Access: inputOutput.
   * @details rolloffFactor indicates how quickly volume is reduced as source
   * moves further from listener.
   */
  void setRolloffFactor(const SFFloat &value) {

    validateRolloffFactor(value);

    _rolloffFactor = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRolloffFactor() for range validation.
   */
  void setRolloffFactorUnchecked(const SFFloat &value) {
    _rolloffFactor = value;
  }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether to spatialize sound playback relative to viewer.
   */
  const SFBool &getSpatialize() const { return _spatialize; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSpatializeUnchecked(const SFBool &value) { _spatialize = value; }

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
  static void checkRangesConeInnerAngle(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesConeOuterAngle(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesIntensity(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxDistance(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesPriority(const SFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesReferenceDistance(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRolloffFactor(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateConeInnerAngle(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("coneInnerAngle below minimum of 0");
    if (value > 6.2832f)
      throw std::out_of_range("coneInnerAngle above maximum of 6.2832");
  }

  static void validateConeOuterAngle(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("coneOuterAngle below minimum of 0");
    if (value > 6.2832f)
      throw std::out_of_range("coneOuterAngle above maximum of 6.2832");
  }

  static void validateIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("intensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("intensity above maximum of 1");
  }

  static void validateMaxDistance(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("maxDistance below minimum of 0");
  }

  static void validatePriority(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("priority below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("priority above maximum of 1");
  }

  static void validateReferenceDistance(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("referenceDistance below minimum of 0");
  }

  static void validateRolloffFactor(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("rolloffFactor below minimum of 0");
  }

  MFNode _children{};

  SFFloat _coneInnerAngle{6.2832};

  SFFloat _coneOuterAngle{6.2832};

  SFFloat _coneOuterGain{0};

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  DistanceModelChoices _distanceModel{DistanceModelChoices::INVERSE};

  SFBool _dopplerEnabled{false};

  SFBool _enableHRTF{false};

  SFFloat _gain{1};

  SFFloat _intensity{1};

  SFVec3f _location{SFVec3f{0, 0, 0}};

  SFFloat _maxDistance{10000};

  SFFloat _priority{0};

  SFFloat _referenceDistance{1};

  SFFloat _rolloffFactor{1};

  SFBool _spatialize{true};
};

} // namespace x3d::nodes
