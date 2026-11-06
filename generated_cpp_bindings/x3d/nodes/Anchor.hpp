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

#include "x3d/nodes/X3DGroupingNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Anchor
 * @brief Anchor is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#Anchor
 */
class Anchor : public virtual X3DGroupingNode, public virtual X3DUrlObject {
public:
  Anchor() = default;

  ~Anchor() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 2; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details If provided, parameter tells the X3D player where to to redirect
   * the loaded url.
   */
  const MFString &getParameter() const { return _parameter; }

  /**
   * Access: inputOutput.
   * @details If provided, parameter tells the X3D player where to to redirect
   * the loaded url.
   */
  void setParameter(const MFString &value) { _parameter = value; }

  void setParameter(MFString &&value) { _parameter = std::move(value); }

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
  MFString _parameter{};
};

} // namespace x3d::nodes
