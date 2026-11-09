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

#include "x3d/nodes/X3DInfoNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class WorldInfo
 * @brief WorldInfo contains a title and simple persistent metadata information
 * about an X3D scene. This node is strictly for documentation purposes and has
 * no effect on the visual appearance or behaviour of the world.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#WorldInfo
 */
class WorldInfo : public virtual X3DInfoNode {
public:
  WorldInfo() = default;

  ~WorldInfo() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Additional information about this model.
   */
  const MFString &getInfo() const { return _info; }

  /**
   * Access: inputOutput.
   * @details Additional information about this model.
   */
  void setInfo(const MFString &value) { _info = value; }

  void setInfo(MFString &&value) { _info = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details title of this world, placed in window title.
   */
  const SFString &getTitle() const { return _title; }

  /**
   * Access: inputOutput.
   * @details title of this world, placed in window title.
   */
  void setTitle(const SFString &value) { _title = value; }

  void setTitle(SFString &&value) { _title = std::move(value); }

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
  MFString _info{};

  SFString _title{};
};

} // namespace x3d::nodes
