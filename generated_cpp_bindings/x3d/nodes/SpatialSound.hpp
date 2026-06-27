// SpatialSound.hpp
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
  /**
   * @brief Default constructor for SpatialSound
   */
  SpatialSound() = default;

  /**
   * @brief Destructor for SpatialSound
   */
  ~SpatialSound() = default;

  /**
   * @brief Get the default value for coneInnerAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultConeInnerAngle() { return 6.2832; }

  /**
   * @brief Get the default value for coneOuterAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultConeOuterAngle() { return 6.2832; }

  /**
   * @brief Get the default value for coneOuterGain
   * @return SFFloat The default value
   */
  static SFFloat getDefaultConeOuterGain() { return 0; }

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

  /**
   * @brief Get the default value for distanceModel
   * @return DistanceModelChoices The default value
   */
  static DistanceModelChoices getDefaultDistanceModel() {
    return DistanceModelChoices::INVERSE;
  }

  /**
   * @brief Get the default value for dopplerEnabled
   * @return SFBool The default value
   */
  static SFBool getDefaultDopplerEnabled() { return false; }

  /**
   * @brief Get the default value for enableHRTF
   * @return SFBool The default value
   */
  static SFBool getDefaultEnableHRTF() { return false; }

  /**
   * @brief Get the default value for gain
   * @return SFFloat The default value
   */
  static SFFloat getDefaultGain() { return 1; }

  /**
   * @brief Get the default value for intensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultIntensity() { return 1; }

  /**
   * @brief Get the default value for location
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for maxDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxDistance() { return 10000; }

  /**
   * @brief Get the default value for priority
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPriority() { return 0; }

  /**
   * @brief Get the default value for referenceDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultReferenceDistance() { return 1; }

  /**
   * @brief Get the default value for rolloffFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRolloffFactor() { return 1; }

  /**
   * @brief Get the default value for spatialize
   * @return SFBool The default value
   */
  static SFBool getDefaultSpatialize() { return true; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

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
   * @brief Gets the value of coneInnerAngle. AccessType: inputOutput
   * @details coneInnerAngle is centered along direction and defines the inner
   * conical volume, inside of which no source gain reduction occurs.
   * @return SFFloat The current value of coneInnerAngle.
   */
  SFFloat getConeInnerAngle() const { return _coneInnerAngle; }

  /**
   * @brief Sets the value of coneInnerAngle. AccessType: inputOutput
   * @details coneInnerAngle is centered along direction and defines the inner
   * conical volume, inside of which no source gain reduction occurs.
   * @param value The new value for coneInnerAngle.
   */
  void setConeInnerAngle(const SFFloat &value) {

    validateConeInnerAngle(value);

    _coneInnerAngle = value;
  }

  /**
   * @brief Non-validating write of coneInnerAngle (runtime/reader ingest path).
   * @details Assigns without the range check setConeInnerAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setConeInnerAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setConeInnerAngleUnchecked(const SFFloat &value) {
    _coneInnerAngle = value;
  }

  /**
   * @brief Gets the value of coneOuterAngle. AccessType: inputOutput
   * @details coneOuterAngle is centered along direction and defines an outer
   * conical volume, within which the sound gain decreases linearly from full
   * gain to coneOuterGain.
   * @return SFFloat The current value of coneOuterAngle.
   */
  SFFloat getConeOuterAngle() const { return _coneOuterAngle; }

  /**
   * @brief Sets the value of coneOuterAngle. AccessType: inputOutput
   * @details coneOuterAngle is centered along direction and defines an outer
   * conical volume, within which the sound gain decreases linearly from full
   * gain to coneOuterGain.
   * @param value The new value for coneOuterAngle.
   */
  void setConeOuterAngle(const SFFloat &value) {

    validateConeOuterAngle(value);

    _coneOuterAngle = value;
  }

  /**
   * @brief Non-validating write of coneOuterAngle (runtime/reader ingest path).
   * @details Assigns without the range check setConeOuterAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setConeOuterAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setConeOuterAngleUnchecked(const SFFloat &value) {
    _coneOuterAngle = value;
  }

  /**
   * @brief Gets the value of coneOuterGain. AccessType: inputOutput
   * @details coneOuterGain is minimum gain value found outside coneOuterAngle.
   * @return SFFloat The current value of coneOuterGain.
   */
  SFFloat getConeOuterGain() const { return _coneOuterGain; }

  /**
   * @brief Sets the value of coneOuterGain. AccessType: inputOutput
   * @details coneOuterGain is minimum gain value found outside coneOuterAngle.
   * @param value The new value for coneOuterGain.
   */
  void setConeOuterGain(const SFFloat &value) { _coneOuterGain = value; }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details direction of sound axis, relative to local coordinate system.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details direction of sound axis, relative to local coordinate system.
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * @brief Gets the value of distanceModel. AccessType: inputOutput
   * @details distanceModel determines how field specifies which algorithm to
   * use for sound attenuation, corresponding to distance between an audio
   * source and a listener, as it moves away from the listener.
   * @return DistanceModelChoices The current value of distanceModel.
   */
  DistanceModelChoices getDistanceModel() const { return _distanceModel; }

  /**
   * @brief Sets the value of distanceModel. AccessType: inputOutput
   * @details distanceModel determines how field specifies which algorithm to
   * use for sound attenuation, corresponding to distance between an audio
   * source and a listener, as it moves away from the listener.
   * @param value The new value for distanceModel.
   */
  void setDistanceModel(const DistanceModelChoices &value) {

    _distanceModel = value;
  }

  /**
   * @brief Gets the value of dopplerEnabled. AccessType: inputOutput
   * @details dopplerEnabled enables/disables whether real-time Doppler effects
   * (due to relation motion between sources and listeners) are computed by
   * browser between virtual sound sources and active listening locations, then
   * applied to received frequency at active listening locations.
   * @return SFBool The current value of dopplerEnabled.
   */
  SFBool getDopplerEnabled() const { return _dopplerEnabled; }

  /**
   * @brief Sets the value of dopplerEnabled. AccessType: inputOutput
   * @details dopplerEnabled enables/disables whether real-time Doppler effects
   * (due to relation motion between sources and listeners) are computed by
   * browser between virtual sound sources and active listening locations, then
   * applied to received frequency at active listening locations.
   * @param value The new value for dopplerEnabled.
   */
  void setDopplerEnabled(const SFBool &value) { _dopplerEnabled = value; }

  /**
   * @brief Gets the value of enableHRTF. AccessType: inputOutput
   * @details enableHRTF enables/disables Head Related Transfer Function (HRTF)
   * auralization, if available.
   * @return SFBool The current value of enableHRTF.
   */
  SFBool getEnableHRTF() const { return _enableHRTF; }

  /**
   * @brief Sets the value of enableHRTF. AccessType: inputOutput
   * @details enableHRTF enables/disables Head Related Transfer Function (HRTF)
   * auralization, if available.
   * @param value The new value for enableHRTF.
   */
  void setEnableHRTF(const SFBool &value) { _enableHRTF = value; }

  /**
   * @brief Gets the value of gain. AccessType: inputOutput
   * @details The gain field is a factor that represents the amount of linear
   * amplification to apply to the output of the node.
   * @return SFFloat The current value of gain.
   */
  SFFloat getGain() const { return _gain; }

  /**
   * @brief Sets the value of gain. AccessType: inputOutput
   * @details The gain field is a factor that represents the amount of linear
   * amplification to apply to the output of the node.
   * @param value The new value for gain.
   */
  void setGain(const SFFloat &value) { _gain = value; }

  /**
   * @brief Gets the value of intensity. AccessType: inputOutput
   * @details Factor [0,1] adjusting loudness (decibels) of emitted sound.
   * @return SFFloat The current value of intensity.
   */
  SFFloat getIntensity() const { return _intensity; }

  /**
   * @brief Sets the value of intensity. AccessType: inputOutput
   * @details Factor [0,1] adjusting loudness (decibels) of emitted sound.
   * @param value The new value for intensity.
   */
  void setIntensity(const SFFloat &value) {

    validateIntensity(value);

    _intensity = value;
  }

  /**
   * @brief Non-validating write of intensity (runtime/reader ingest path).
   * @details Assigns without the range check setIntensity() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setIntensity() stays the
   *          enforcement point for programmatic callers.
   */
  void setIntensityUnchecked(const SFFloat &value) { _intensity = value; }

  /**
   * @brief Gets the value of location. AccessType: inputOutput
   * @details Position of sound ellipsoid center, relative to local coordinate
   * system.
   * @return SFVec3f The current value of location.
   */
  SFVec3f getLocation() const { return _location; }

  /**
   * @brief Sets the value of location. AccessType: inputOutput
   * @details Position of sound ellipsoid center, relative to local coordinate
   * system.
   * @param value The new value for location.
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * @brief Gets the value of maxDistance. AccessType: inputOutput
   * @details maxDistance is the maximum distance where sound is renderable
   * between source and listener, after which no reduction in sound volume
   * occurs.
   * @return SFFloat The current value of maxDistance.
   */
  SFFloat getMaxDistance() const { return _maxDistance; }

  /**
   * @brief Sets the value of maxDistance. AccessType: inputOutput
   * @details maxDistance is the maximum distance where sound is renderable
   * between source and listener, after which no reduction in sound volume
   * occurs.
   * @param value The new value for maxDistance.
   */
  void setMaxDistance(const SFFloat &value) {

    validateMaxDistance(value);

    _maxDistance = value;
  }

  /**
   * @brief Non-validating write of maxDistance (runtime/reader ingest path).
   * @details Assigns without the range check setMaxDistance() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxDistance() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxDistanceUnchecked(const SFFloat &value) { _maxDistance = value; }

  /**
   * @brief Gets the value of priority. AccessType: inputOutput
   * @details Player hint [0,1] if needed to choose which sounds to play.
   * @return SFFloat The current value of priority.
   */
  SFFloat getPriority() const { return _priority; }

  /**
   * @brief Sets the value of priority. AccessType: inputOutput
   * @details Player hint [0,1] if needed to choose which sounds to play.
   * @param value The new value for priority.
   */
  void setPriority(const SFFloat &value) {

    validatePriority(value);

    _priority = value;
  }

  /**
   * @brief Non-validating write of priority (runtime/reader ingest path).
   * @details Assigns without the range check setPriority() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setPriority() stays the
   *          enforcement point for programmatic callers.
   */
  void setPriorityUnchecked(const SFFloat &value) { _priority = value; }

  /**
   * @brief Gets the value of referenceDistance. AccessType: inputOutput
   * @details referenceDistance for reducing volume as source moves further from
   * the listener.
   * @return SFFloat The current value of referenceDistance.
   */
  SFFloat getReferenceDistance() const { return _referenceDistance; }

  /**
   * @brief Sets the value of referenceDistance. AccessType: inputOutput
   * @details referenceDistance for reducing volume as source moves further from
   * the listener.
   * @param value The new value for referenceDistance.
   */
  void setReferenceDistance(const SFFloat &value) {

    validateReferenceDistance(value);

    _referenceDistance = value;
  }

  /**
   * @brief Non-validating write of referenceDistance (runtime/reader ingest
   * path).
   * @details Assigns without the range check setReferenceDistance() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setReferenceDistance() stays the
   *          enforcement point for programmatic callers.
   */
  void setReferenceDistanceUnchecked(const SFFloat &value) {
    _referenceDistance = value;
  }

  /**
   * @brief Gets the value of rolloffFactor. AccessType: inputOutput
   * @details rolloffFactor indicates how quickly volume is reduced as source
   * moves further from listener.
   * @return SFFloat The current value of rolloffFactor.
   */
  SFFloat getRolloffFactor() const { return _rolloffFactor; }

  /**
   * @brief Sets the value of rolloffFactor. AccessType: inputOutput
   * @details rolloffFactor indicates how quickly volume is reduced as source
   * moves further from listener.
   * @param value The new value for rolloffFactor.
   */
  void setRolloffFactor(const SFFloat &value) {

    validateRolloffFactor(value);

    _rolloffFactor = value;
  }

  /**
   * @brief Non-validating write of rolloffFactor (runtime/reader ingest path).
   * @details Assigns without the range check setRolloffFactor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRolloffFactor() stays the
   *          enforcement point for programmatic callers.
   */
  void setRolloffFactorUnchecked(const SFFloat &value) {
    _rolloffFactor = value;
  }

  /**
   * @brief Gets the value of spatialize. AccessType: initializeOnly
   * @details Whether to spatialize sound playback relative to viewer.
   * @return SFBool The current value of spatialize.
   */
  SFBool getSpatialize() const { return _spatialize; }
  /**
   * @brief Data-layer write of spatialize (reader/init ingest path).
   * @details spatialize is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSpatialize().
   */
  void setSpatializeUnchecked(const SFBool &value) { _spatialize = value; }

  /**
   * @brief The X3D type name of this node (e.g. "SpatialSound").
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

    if (value < 0)
      throw std::out_of_range("coneInnerAngle below minimum of 0");
    if (value > 6.2832)
      throw std::out_of_range("coneInnerAngle above maximum of 6.2832");
  }

  static void validateConeOuterAngle(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("coneOuterAngle below minimum of 0");
    if (value > 6.2832)
      throw std::out_of_range("coneOuterAngle above maximum of 6.2832");
  }

  static void validateIntensity(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("intensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("intensity above maximum of 1");
  }

  static void validateMaxDistance(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("maxDistance below minimum of 0");
  }

  static void validatePriority(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("priority below minimum of 0");
    if (value > 1)
      throw std::out_of_range("priority above maximum of 1");
  }

  static void validateReferenceDistance(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("referenceDistance below minimum of 0");
  }

  static void validateRolloffFactor(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("rolloffFactor below minimum of 0");
  }

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for coneInnerAngle.
   */

  SFFloat _coneInnerAngle{6.2832};

  /**
   * @brief Member variable for coneOuterAngle.
   */

  SFFloat _coneOuterAngle{6.2832};

  /**
   * @brief Member variable for coneOuterGain.
   */

  SFFloat _coneOuterGain{0};

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  /**
   * @brief Member variable for distanceModel.
   */

  DistanceModelChoices _distanceModel{DistanceModelChoices::INVERSE};

  /**
   * @brief Member variable for dopplerEnabled.
   */

  SFBool _dopplerEnabled{false};

  /**
   * @brief Member variable for enableHRTF.
   */

  SFBool _enableHRTF{false};

  /**
   * @brief Member variable for gain.
   */

  SFFloat _gain{1};

  /**
   * @brief Member variable for intensity.
   */

  SFFloat _intensity{1};

  /**
   * @brief Member variable for location.
   */

  SFVec3f _location{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for maxDistance.
   */

  SFFloat _maxDistance{10000};

  /**
   * @brief Member variable for priority.
   */

  SFFloat _priority{0};

  /**
   * @brief Member variable for referenceDistance.
   */

  SFFloat _referenceDistance{1};

  /**
   * @brief Member variable for rolloffFactor.
   */

  SFFloat _rolloffFactor{1};

  /**
   * @brief Member variable for spatialize.
   */

  SFBool _spatialize{true};
};

} // namespace x3d::nodes
