// PeriodicWave.hpp
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
  /**
   * @brief Default constructor for PeriodicWave
   */
  PeriodicWave() = default;

  /**
   * @brief Destructor for PeriodicWave
   */
  ~PeriodicWave() = default;

  /**
   * @brief Get the default value for type
   * @return PeriodicWaveTypeChoices The default value
   */
  static PeriodicWaveTypeChoices getDefaultType() {
    return PeriodicWaveTypeChoices::SQUARE;
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "periodicWave"; }

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
   * @brief Gets the value of optionsImag. AccessType: inputOutput
   * @details imaginary coefficients for defining a waveform.
   * @return MFFloat The current value of optionsImag.
   */
  MFFloat getOptionsImag() const { return _optionsImag; }

  /**
   * @brief Sets the value of optionsImag. AccessType: inputOutput
   * @details imaginary coefficients for defining a waveform.
   * @param value The new value for optionsImag.
   */
  void setOptionsImag(const MFFloat &value) { _optionsImag = value; }

  void setOptionsImag(MFFloat &&value) { _optionsImag = std::move(value); }

  /**
   * @brief Gets the value of optionsReal. AccessType: inputOutput
   * @details real coefficients for defining a waveform.
   * @return MFFloat The current value of optionsReal.
   */
  MFFloat getOptionsReal() const { return _optionsReal; }

  /**
   * @brief Sets the value of optionsReal. AccessType: inputOutput
   * @details real coefficients for defining a waveform.
   * @param value The new value for optionsReal.
   */
  void setOptionsReal(const MFFloat &value) { _optionsReal = value; }

  void setOptionsReal(MFFloat &&value) { _optionsReal = std::move(value); }

  /**
   * @brief Gets the value of type. AccessType: inputOutput
   * @details The type field specifies shape of waveform to play, which can be
   * one of several provided values or else 'custom' to indicate that real and
   * imaginary coefficient arrays define a custom waveform.
   * @return PeriodicWaveTypeChoices The current value of type.
   */
  PeriodicWaveTypeChoices getType() const { return _type; }

  /**
   * @brief Sets the value of type. AccessType: inputOutput
   * @details The type field specifies shape of waveform to play, which can be
   * one of several provided values or else 'custom' to indicate that real and
   * imaginary coefficient arrays define a custom waveform.
   * @param value The new value for type.
   */
  void setType(const PeriodicWaveTypeChoices &value) { _type = value; }

  /**
   * @brief The X3D type name of this node (e.g. "PeriodicWave").
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

private:
  /**
   * @brief Member variable for optionsImag.
   */

  MFFloat _optionsImag{};

  /**
   * @brief Member variable for optionsReal.
   */

  MFFloat _optionsReal{};

  /**
   * @brief Member variable for type.
   */

  PeriodicWaveTypeChoices _type{PeriodicWaveTypeChoices::SQUARE};
};

} // namespace x3d::nodes
