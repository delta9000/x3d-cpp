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

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class FontLibrary
 * @brief FontLibrary can load additional fonts for use by Text and FontStyle
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/text.html#FontStyle
 */
class FontLibrary : public virtual X3DChildNode, public virtual X3DUrlObject {
public:
  FontLibrary() = default;

  ~FontLibrary() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Text"; }

  static int componentLevel() { return 2; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   */
  const MFString &getFamily() const { return _family; }

  /**
   * Access: inputOutput.
   * @details Array of quoted font family names in preference order, browsers
   * use the first supported family.
   */
  void setFamily(const MFString &value) { _family = value; }

  void setFamily(MFString &&value) { _family = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  MFString _family{};
};

} // namespace x3d::nodes
