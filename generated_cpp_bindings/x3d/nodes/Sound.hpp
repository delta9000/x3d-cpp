// Sound.hpp
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
 * @class Sound
 * @brief The Sound node controls the 3D spatialization of sound playback by a
 * child AudioClip or MovieTexture node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#Sound
 */
class Sound : public virtual X3DSoundNode {
public:
  /**
   * @brief Default constructor for Sound
   */
  Sound() = default;

  /**
   * @brief Destructor for Sound
   */
  ~Sound() = default;

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

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
   * @brief Get the default value for maxBack
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxBack() { return 10; }

  /**
   * @brief Get the default value for maxFront
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxFront() { return 10; }

  /**
   * @brief Get the default value for minBack
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinBack() { return 1; }

  /**
   * @brief Get the default value for minFront
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinFront() { return 1; }

  /**
   * @brief Get the default value for priority
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPriority() { return 0; }

  /**
   * @brief Get the default value for source
   * @return SFNode The default value
   */
  static SFNode getDefaultSource() { return nullptr; }

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
  static int componentLevel() { return 1; }

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
   * @brief Gets the value of maxBack. AccessType: inputOutput
   * @details Outer (zero loudness)ellipsoid distance along back direction.
   * @return SFFloat The current value of maxBack.
   */
  SFFloat getMaxBack() const { return _maxBack; }

  /**
   * @brief Sets the value of maxBack. AccessType: inputOutput
   * @details Outer (zero loudness)ellipsoid distance along back direction.
   * @param value The new value for maxBack.
   */
  void setMaxBack(const SFFloat &value) {

    validateMaxBack(value);

    _maxBack = value;
  }

  /**
   * @brief Non-validating write of maxBack (runtime/reader ingest path).
   * @details Assigns without the range check setMaxBack() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxBack() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxBackUnchecked(const SFFloat &value) { _maxBack = value; }

  /**
   * @brief Gets the value of maxFront. AccessType: inputOutput
   * @details Outer (zero loudness)ellipsoid distance along front direction.
   * @return SFFloat The current value of maxFront.
   */
  SFFloat getMaxFront() const { return _maxFront; }

  /**
   * @brief Sets the value of maxFront. AccessType: inputOutput
   * @details Outer (zero loudness)ellipsoid distance along front direction.
   * @param value The new value for maxFront.
   */
  void setMaxFront(const SFFloat &value) {

    validateMaxFront(value);

    _maxFront = value;
  }

  /**
   * @brief Non-validating write of maxFront (runtime/reader ingest path).
   * @details Assigns without the range check setMaxFront() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxFront() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxFrontUnchecked(const SFFloat &value) { _maxFront = value; }

  /**
   * @brief Gets the value of minBack. AccessType: inputOutput
   * @details Inner (full loudness) ellipsoid distance along back direction.
   * @return SFFloat The current value of minBack.
   */
  SFFloat getMinBack() const { return _minBack; }

  /**
   * @brief Sets the value of minBack. AccessType: inputOutput
   * @details Inner (full loudness) ellipsoid distance along back direction.
   * @param value The new value for minBack.
   */
  void setMinBack(const SFFloat &value) {

    validateMinBack(value);

    _minBack = value;
  }

  /**
   * @brief Non-validating write of minBack (runtime/reader ingest path).
   * @details Assigns without the range check setMinBack() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMinBack() stays the
   *          enforcement point for programmatic callers.
   */
  void setMinBackUnchecked(const SFFloat &value) { _minBack = value; }

  /**
   * @brief Gets the value of minFront. AccessType: inputOutput
   * @details Inner (full loudness) ellipsoid distance along front direction.
   * @return SFFloat The current value of minFront.
   */
  SFFloat getMinFront() const { return _minFront; }

  /**
   * @brief Sets the value of minFront. AccessType: inputOutput
   * @details Inner (full loudness) ellipsoid distance along front direction.
   * @param value The new value for minFront.
   */
  void setMinFront(const SFFloat &value) {

    validateMinFront(value);

    _minFront = value;
  }

  /**
   * @brief Non-validating write of minFront (runtime/reader ingest path).
   * @details Assigns without the range check setMinFront() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMinFront() stays the
   *          enforcement point for programmatic callers.
   */
  void setMinFrontUnchecked(const SFFloat &value) { _minFront = value; }

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
   * @brief Gets the value of source. AccessType: inputOutput
   * @details sound source for the Sound node, either an AudioClip node or a
   * MovieTexture node.
   * @return SFNode The current value of source.
   */
  SFNode getSource() const { return _source; }

  /**
   * @brief Acceptable node types for the source field.
   * @details Permitted X3D node types: X3DSoundSourceNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSourceNodeTypes() {
    static const std::vector<std::string> types = {"X3DSoundSourceNode"};
    return types;
  }

  /**
   * @brief Sets the value of source. AccessType: inputOutput
   * @details sound source for the Sound node, either an AudioClip node or a
   * MovieTexture node.
   * @param value The new value for source.
   */
  void setSource(const SFNode &value) { _source = value; }

  void setSource(SFNode &&value) { _source = std::move(value); }

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
   * @brief The X3D type name of this node (e.g. "Sound").
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
  static void checkRangesIntensity(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxBack(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxFront(const SFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMinBack(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMinFront(const SFFloat &value,
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

private:
  static void validateIntensity(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("intensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("intensity above maximum of 1");
  }

  static void validateMaxBack(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("maxBack below minimum of 0");
  }

  static void validateMaxFront(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("maxFront below minimum of 0");
  }

  static void validateMinBack(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("minBack below minimum of 0");
  }

  static void validateMinFront(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("minFront below minimum of 0");
  }

  static void validatePriority(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("priority below minimum of 0");
    if (value > 1)
      throw std::out_of_range("priority above maximum of 1");
  }

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  /**
   * @brief Member variable for intensity.
   */

  SFFloat _intensity{1};

  /**
   * @brief Member variable for location.
   */

  SFVec3f _location{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for maxBack.
   */

  SFFloat _maxBack{10};

  /**
   * @brief Member variable for maxFront.
   */

  SFFloat _maxFront{10};

  /**
   * @brief Member variable for minBack.
   */

  SFFloat _minBack{1};

  /**
   * @brief Member variable for minFront.
   */

  SFFloat _minFront{1};

  /**
   * @brief Member variable for priority.
   */

  SFFloat _priority{0};

  /**
   * @brief Member variable for source.
   */

  SFNode _source{nullptr};

  /**
   * @brief Member variable for spatialize.
   */

  SFBool _spatialize{true};
};

} // namespace x3d::nodes
