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
  OscillatorSource() = default;

  ~OscillatorSource() = default;

  static SFFloat getDefaultDetune() { return 0; }

  static SFFloat getDefaultFrequency() { return 440; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The detune ffield is an a-rate AudioParam representing detuning of
   * oscillation in cents (though the AudioParam returned is read-only, the
   * value it represents is not).
   */
  const SFFloat &getDetune() const { return _detune; }

  /**
   * Access: inputOutput.
   * @details The detune ffield is an a-rate AudioParam representing detuning of
   * oscillation in cents (though the AudioParam returned is read-only, the
   * value it represents is not).
   */
  void setDetune(const SFFloat &value) {

    validateDetune(value);

    _detune = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDetune() for range validation.
   */
  void setDetuneUnchecked(const SFFloat &value) { _detune = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The frequency of oscillation in hertz.
   */
  const SFFloat &getFrequency() const { return _frequency; }

  /**
   * Access: inputOutput.
   * @details The frequency of oscillation in hertz.
   */
  void setFrequency(const SFFloat &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrequency() for range validation.
   */
  void setFrequencyUnchecked(const SFFloat &value) { _frequency = value; }

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

    if (value < 0.0f)
      throw std::out_of_range("detune below minimum of 0");
  }

  static void validateFrequency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("frequency below minimum of 0");
  }

  SFFloat _detune{0};

  SFFloat _frequency{440};
};

} // namespace x3d::nodes
