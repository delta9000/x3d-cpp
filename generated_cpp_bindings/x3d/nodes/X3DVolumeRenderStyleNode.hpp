#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DVolumeRenderStyleNode
 * @brief The X3DVolumeRenderStyleNode abstract node type is the base type for
 * all node types that specify a specific visual rendering style to be used when
 * rendering volume data.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#X3DVolumeRenderStyleNode
 */
class X3DVolumeRenderStyleNode : public virtual X3DNode {
public:
  X3DVolumeRenderStyleNode() = default;

  virtual ~X3DVolumeRenderStyleNode() = default;

  static SFBool getDefaultEnabled() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "VolumeRendering"; }

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
};

} // namespace x3d::nodes
