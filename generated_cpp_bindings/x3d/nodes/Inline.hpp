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

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Inline
 * @brief Inline can load another X3D or VRML model into the current scene via
 * url.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#Inline
 */
class Inline : public virtual X3DChildNode,
               public virtual X3DBoundedObject,
               public virtual X3DUrlObject {
public:
  Inline() = default;

  ~Inline() = default;

  static SFBool getDefaultGlobal() { return false; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 2; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details The global field controls potential external scoping effects of
   * lights found within an Inline scene.
   */
  const SFBool &getGlobal() const { return _global; }

  /**
   * Access: inputOutput.
   * @details The global field controls potential external scoping effects of
   * lights found within an Inline scene.
   */
  void setGlobal(const SFBool &value) { _global = value; }

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
  SFBool _global{false};
};

} // namespace x3d::nodes
