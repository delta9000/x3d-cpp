// OscillatorSource.hpp
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
 * @class OscillatorSource
 * @brief OscillatorSource node represents an audio source generating a periodic
 * waveform, providing a constant tone.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#OscillatorSource
 */
class OscillatorSource : public virtual X3DSoundSourceNode {
public:
  /**
   * @brief Default constructor for OscillatorSource
   */
  OscillatorSource() = default;

  /**
   * @brief Destructor for OscillatorSource
   */
  ~OscillatorSource() = default;

  /**
   * @brief Get the default value for detune
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDetune() { return 0; }

  /**
   * @brief Get the default value for frequency
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFrequency() { return 440; }

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
   * @brief Gets the value of detune. AccessType: inputOutput
   * @details The detune ffield is an a-rate AudioParam representing detuning of
   * oscillation in cents (though the AudioParam returned is read-only, the
   * value it represents is not).
   * @return SFFloat The current value of detune.
   */
  SFFloat getDetune() const { return _detune; }

  /**
   * @brief Sets the value of detune. AccessType: inputOutput
   * @details The detune ffield is an a-rate AudioParam representing detuning of
   * oscillation in cents (though the AudioParam returned is read-only, the
   * value it represents is not).
   * @param value The new value for detune.
   */
  void setDetune(const SFFloat &value) {

    validateDetune(value);

    _detune = value;
  }

  /**
   * @brief Non-validating write of detune (runtime/reader ingest path).
   * @details Assigns without the range check setDetune() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDetune() stays the
   *          enforcement point for programmatic callers.
   */
  void setDetuneUnchecked(const SFFloat &value) { _detune = value; }

  /**
   * @brief Gets the value of frequency. AccessType: inputOutput
   * @details The frequency of oscillation in hertz.
   * @return SFFloat The current value of frequency.
   */
  SFFloat getFrequency() const { return _frequency; }

  /**
   * @brief Sets the value of frequency. AccessType: inputOutput
   * @details The frequency of oscillation in hertz.
   * @param value The new value for frequency.
   */
  void setFrequency(const SFFloat &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * @brief Non-validating write of frequency (runtime/reader ingest path).
   * @details Assigns without the range check setFrequency() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFrequency() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrequencyUnchecked(const SFFloat &value) { _frequency = value; }

  /**
   * @brief The X3D type name of this node (e.g. "OscillatorSource").
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
  static void checkRangesDetune(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrequency(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateDetune(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("detune below minimum of 0");
  }

  static void validateFrequency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("frequency below minimum of 0");
  }

  /**
   * @brief Member variable for detune.
   */

  SFFloat _detune{0};

  /**
   * @brief Member variable for frequency.
   */

  SFFloat _frequency{440};
};

} // namespace x3d::nodes
