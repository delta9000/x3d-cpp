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

#include "x3d/nodes/X3DFogObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class LocalFog
 * @brief LocalFog simulates atmospheric effects by blending distant objects
 * with fog color.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#LocalFog
 */
class LocalFog : public virtual X3DChildNode, public virtual X3DFogObject {
public:
  LocalFog() = default;

  ~LocalFog() = default;

  static SFBool getDefaultEnabled() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 4; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables node operation.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables node operation.
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFBool _enabled{true};
};

} // namespace x3d::nodes
