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
 * @class PeriodicWave
 * @brief PeriodicWave defines a periodic waveform that can be used to shape the
 * output of an Oscillator.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#PeriodicWave
 */
class PeriodicWave : public virtual X3DSoundNode {
public:
  PeriodicWave() = default;

  ~PeriodicWave() = default;

  static PeriodicWaveTypeChoices getDefaultType() {
    return PeriodicWaveTypeChoices::SQUARE;
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "periodicWave"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details imaginary coefficients for defining a waveform.
   */
  const MFFloat &getOptionsImag() const { return _optionsImag; }

  /**
   * Access: inputOutput.
   * @details imaginary coefficients for defining a waveform.
   */
  void setOptionsImag(const MFFloat &value) { _optionsImag = value; }

  void setOptionsImag(MFFloat &&value) { _optionsImag = std::move(value); }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details real coefficients for defining a waveform.
   */
  const MFFloat &getOptionsReal() const { return _optionsReal; }

  /**
   * Access: inputOutput.
   * @details real coefficients for defining a waveform.
   */
  void setOptionsReal(const MFFloat &value) { _optionsReal = value; }

  void setOptionsReal(MFFloat &&value) { _optionsReal = std::move(value); }

  /**
   * PeriodicWaveTypeChoices; access: inputOutput. Borrows the stored value.
   * @details The type field specifies shape of waveform to play, which can be
   * one of several provided values or else 'custom' to indicate that real and
   * imaginary coefficient arrays define a custom waveform.
   */
  const PeriodicWaveTypeChoices &getType() const { return _type; }

  /**
   * Access: inputOutput.
   * @details The type field specifies shape of waveform to play, which can be
   * one of several provided values or else 'custom' to indicate that real and
   * imaginary coefficient arrays define a custom waveform.
   */
  void setType(const PeriodicWaveTypeChoices &value) { _type = value; }

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

private:
  MFFloat _optionsImag{};

  MFFloat _optionsReal{};

  PeriodicWaveTypeChoices _type{PeriodicWaveTypeChoices::SQUARE};
};

} // namespace x3d::nodes
