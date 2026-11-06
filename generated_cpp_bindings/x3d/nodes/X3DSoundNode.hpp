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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DSoundNode
 * @brief Base type for all sound nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#X3DSoundNode
 */
class X3DSoundNode : public virtual X3DChildNode {
public:
  X3DSoundNode() = default;

  virtual ~X3DSoundNode() = default;

  static SFBool getDefaultEnabled() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

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
  SFString _description{};

  SFBool _enabled{true};
};

} // namespace x3d::nodes
