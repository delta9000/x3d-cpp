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
 * @class ListenerPointSource
 * @brief ListenerPointSource node represents position and orientation of a
 * person listening to virtual sound in the audio scene, and provides single or
 * multiple sound channels as output.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#ListenerPointSource
 */
class ListenerPointSource : public virtual X3DSoundSourceNode {
public:
  ListenerPointSource() = default;

  ~ListenerPointSource() = default;

  static SFBool getDefaultDopplerEnabled() { return false; }

  static SFFloat getDefaultInterauralDistance() { return 0; }

  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultTrackCurrentView() { return false; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The interauralDistance field is.
   */
  const SFFloat &getInterauralDistance() const { return _interauralDistance; }

  /**
   * Access: inputOutput.
   * @details The interauralDistance field is.
   */
  void setInterauralDistance(const SFFloat &value) {

    validateInterauralDistance(value);

    _interauralDistance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setInterauralDistance() for range validation.
   */
  void setInterauralDistanceUnchecked(const SFFloat &value) {
    _interauralDistance = value;
  }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Rotation (axis, angle in radians) of listening point direction
   * relative to default -Z axis direction in local coordinate system.
   */
  const SFRotation &getOrientation() const { return _orientation; }

  /**
   * Access: inputOutput.
   * @details Rotation (axis, angle in radians) of listening point direction
   * relative to default -Z axis direction in local coordinate system.
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details position (x, y, z in meters) relative to local coordinate system.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details position (x, y, z in meters) relative to local coordinate system.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details If trackCurrentView field is true then position and orientation
   * match avatar's (user's) current view.
   */
  const SFBool &getTrackCurrentView() const { return _trackCurrentView; }

  /**
   * Access: inputOutput.
   * @details If trackCurrentView field is true then position and orientation
   * match avatar's (user's) current view.
   */
  void setTrackCurrentView(const SFBool &value) { _trackCurrentView = value; }

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
  static void checkRangesInterauralDistance(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out);

private:
  static void validateInterauralDistance(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("interauralDistance below minimum of 0");
  }

  SFBool _dopplerEnabled{false};

  SFFloat _interauralDistance{0};

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  SFVec3f _position{SFVec3f{0, 0, 0}};

  SFBool _trackCurrentView{false};
};

} // namespace x3d::nodes
