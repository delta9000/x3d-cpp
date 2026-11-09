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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSoundSourceNode
 * @brief Nodes implementing X3DSoundSourceNode provide signal inputs to the
 * audio graph.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#X3DSoundSourceNode
 */
class X3DSoundSourceNode : public virtual X3DTimeDependentNode,
                           public virtual X3DSoundNode {
public:
  X3DSoundSourceNode() = default;

  virtual ~X3DSoundSourceNode() = default;

  static SFBool getDefaultEnabled() { return true; }

  static SFFloat getDefaultGain() { return 1; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getGain() const { return _gain; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGain(const SFFloat &value) { _gain = value; }

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
  SFBool _enabled{true};

  SFFloat _gain{1};
};

} // namespace x3d::nodes
