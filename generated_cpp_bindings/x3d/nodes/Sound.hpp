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
  Sound() = default;

  ~Sound() = default;

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, 1}; }

  static SFFloat getDefaultIntensity() { return 1; }

  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultMaxBack() { return 10; }

  static SFFloat getDefaultMaxFront() { return 10; }

  static SFFloat getDefaultMinBack() { return 1; }

  static SFFloat getDefaultMinFront() { return 1; }

  static SFFloat getDefaultPriority() { return 0; }

  static SFNode getDefaultSource() { return nullptr; }

  static SFBool getDefaultSpatialize() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 1; }

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
   * @details Outer (zero loudness)ellipsoid distance along back direction.
   */
  const SFFloat &getMaxBack() const { return _maxBack; }

  /**
   * Access: inputOutput.
   * @details Outer (zero loudness)ellipsoid distance along back direction.
   */
  void setMaxBack(const SFFloat &value) {

    validateMaxBack(value);

    _maxBack = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxBack() for range validation.
   */
  void setMaxBackUnchecked(const SFFloat &value) { _maxBack = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Outer (zero loudness)ellipsoid distance along front direction.
   */
  const SFFloat &getMaxFront() const { return _maxFront; }

  /**
   * Access: inputOutput.
   * @details Outer (zero loudness)ellipsoid distance along front direction.
   */
  void setMaxFront(const SFFloat &value) {

    validateMaxFront(value);

    _maxFront = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxFront() for range validation.
   */
  void setMaxFrontUnchecked(const SFFloat &value) { _maxFront = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Inner (full loudness) ellipsoid distance along back direction.
   */
  const SFFloat &getMinBack() const { return _minBack; }

  /**
   * Access: inputOutput.
   * @details Inner (full loudness) ellipsoid distance along back direction.
   */
  void setMinBack(const SFFloat &value) {

    validateMinBack(value);

    _minBack = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMinBack() for range validation.
   */
  void setMinBackUnchecked(const SFFloat &value) { _minBack = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Inner (full loudness) ellipsoid distance along front direction.
   */
  const SFFloat &getMinFront() const { return _minFront; }

  /**
   * Access: inputOutput.
   * @details Inner (full loudness) ellipsoid distance along front direction.
   */
  void setMinFront(const SFFloat &value) {

    validateMinFront(value);

    _minFront = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMinFront() for range validation.
   */
  void setMinFrontUnchecked(const SFFloat &value) { _minFront = value; }

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
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details sound source for the Sound node, either an AudioClip node or a
   * MovieTexture node.
   */
  const SFNode &getSource() const { return _source; }

  /**
   * @details Permitted X3D node types: X3DSoundSourceNode
   */
  static const std::vector<std::string> &acceptableSourceNodeTypes() {
    static const std::vector<std::string> types = {"X3DSoundSourceNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details sound source for the Sound node, either an AudioClip node or a
   * MovieTexture node.
   */
  void setSource(const SFNode &value) { _source = value; }

  void setSource(SFNode &&value) { _source = std::move(value); }

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

    if (value < 0.0f)
      throw std::out_of_range("intensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("intensity above maximum of 1");
  }

  static void validateMaxBack(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("maxBack below minimum of 0");
  }

  static void validateMaxFront(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("maxFront below minimum of 0");
  }

  static void validateMinBack(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("minBack below minimum of 0");
  }

  static void validateMinFront(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("minFront below minimum of 0");
  }

  static void validatePriority(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("priority below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("priority above maximum of 1");
  }

  SFVec3f _direction{SFVec3f{0, 0, 1}};

  SFFloat _intensity{1};

  SFVec3f _location{SFVec3f{0, 0, 0}};

  SFFloat _maxBack{10};

  SFFloat _maxFront{10};

  SFFloat _minBack{1};

  SFFloat _minFront{1};

  SFFloat _priority{0};

  SFNode _source{nullptr};

  SFBool _spatialize{true};
};

} // namespace x3d::nodes
