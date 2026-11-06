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

#include "x3d/nodes/X3DSequencerNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class IntegerSequencer
 * @brief IntegerSequencer generates periodic discrete integer values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#IntegerSequencer
 */
class IntegerSequencer : public virtual X3DSequencerNode {
public:
  IntegerSequencer() = default;

  ~IntegerSequencer() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Output values for linear sequencing, each corresponding to an
   * input-fraction value in the key array.
   */
  const MFInt32 &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for linear sequencing, each corresponding to an
   * input-fraction value in the key array.
   */
  void setKeyValue(const MFInt32 &value) { _keyValue = value; }

  void setKeyValue(MFInt32 &&value) { _keyValue = std::move(value); }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details Single intermittent output value determined by current key time
   * and corresponding keyValue entry.
   */
  const SFInt32 &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Single intermittent output value determined by current key time
   * and corresponding keyValue entry.
   */
  void emitValue_changed(const SFInt32 &value) { _value_changed = value; }

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
  MFInt32 _keyValue{};

  SFInt32 _value_changed{};
};

} // namespace x3d::nodes
