// ListenerPointSource.hpp
#ifndef LISTENERPOINTSOURCE_HPP
#define LISTENERPOINTSOURCE_HPP

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
 * @class ListenerPointSource
 * @brief ListenerPointSource node represents position and orientation of a
 * person listening to virtual sound in the audio scene, and provides single or
 * multiple sound channels as output.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#ListenerPointSource
 */
class ListenerPointSource : public virtual X3DSoundSourceNode {
public:
  /**
   * @brief Default constructor for ListenerPointSource
   */
  ListenerPointSource() = default;

  /**
   * @brief Destructor for ListenerPointSource
   */
  ~ListenerPointSource() = default;

  /**
   * @brief Get the default value for dopplerEnabled
   * @return SFBool The default value
   */
  static SFBool getDefaultDopplerEnabled() { return false; }

  /**
   * @brief Get the default value for interauralDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultInterauralDistance() { return 0; }

  /**
   * @brief Get the default value for orientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for position
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for trackCurrentView
   * @return SFBool The default value
   */
  static SFBool getDefaultTrackCurrentView() { return false; }

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
   * @brief Gets the value of interauralDistance. AccessType: inputOutput
   * @details The interauralDistance field is.
   * @return SFFloat The current value of interauralDistance.
   */
  SFFloat getInterauralDistance() const { return _interauralDistance; }

  /**
   * @brief Sets the value of interauralDistance. AccessType: inputOutput
   * @details The interauralDistance field is.
   * @param value The new value for interauralDistance.
   */
  void setInterauralDistance(const SFFloat &value) {

    validateInterauralDistance(value);

    _interauralDistance = value;
  }

  /**
   * @brief Non-validating write of interauralDistance (runtime/reader ingest
   * path).
   * @details Assigns without the range check setInterauralDistance() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setInterauralDistance() stays the
   *          enforcement point for programmatic callers.
   */
  void setInterauralDistanceUnchecked(const SFFloat &value) {
    _interauralDistance = value;
  }

  /**
   * @brief Gets the value of orientation. AccessType: inputOutput
   * @details Rotation (axis, angle in radians) of listening point direction
   * relative to default -Z axis direction in local coordinate system.
   * @return SFRotation The current value of orientation.
   */
  SFRotation getOrientation() const { return _orientation; }

  /**
   * @brief Sets the value of orientation. AccessType: inputOutput
   * @details Rotation (axis, angle in radians) of listening point direction
   * relative to default -Z axis direction in local coordinate system.
   * @param value The new value for orientation.
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * @brief Gets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) relative to local coordinate system.
   * @return SFVec3f The current value of position.
   */
  SFVec3f getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) relative to local coordinate system.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * @brief Gets the value of trackCurrentView. AccessType: inputOutput
   * @details If trackCurrentView field is true then position and orientation
   * match avatar's (user's) current view.
   * @return SFBool The current value of trackCurrentView.
   */
  SFBool getTrackCurrentView() const { return _trackCurrentView; }

  /**
   * @brief Sets the value of trackCurrentView. AccessType: inputOutput
   * @details If trackCurrentView field is true then position and orientation
   * match avatar's (user's) current view.
   * @param value The new value for trackCurrentView.
   */
  void setTrackCurrentView(const SFBool &value) { _trackCurrentView = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ListenerPointSource").
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
  static void checkRangesInterauralDistance(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out);

private:
  static void validateInterauralDistance(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("interauralDistance below minimum of 0");
  }

  /**
   * @brief Member variable for dopplerEnabled.
   */

  SFBool _dopplerEnabled{false};

  /**
   * @brief Member variable for interauralDistance.
   */

  SFFloat _interauralDistance{0};

  /**
   * @brief Member variable for orientation.
   */

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for position.
   */

  SFVec3f _position{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for trackCurrentView.
   */

  SFBool _trackCurrentView{false};
};

#endif // LISTENERPOINTSOURCE_HPP