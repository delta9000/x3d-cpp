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

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoMetadata
 * @brief GeoMetadata includes a generic subset of metadata about the geographic
 * data.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoMetadata
 */
class GeoMetadata : public virtual X3DInfoNode, public virtual X3DUrlObject {
public:
  GeoMetadata() = default;

  ~GeoMetadata() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details DEF list of all nodes that implement this data.
   */
  const MFNode &getData() const { return _data; }

  /**
   * @details Permitted X3D node types: X3DNode
   */
  static const std::vector<std::string> &acceptableDataNodeTypes() {
    static const std::vector<std::string> types = {"X3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details DEF list of all nodes that implement this data.
   */
  void setData(const MFNode &value) { _data = value; }

  void setData(MFNode &&value) { _data = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details The summary string array contains a set of keyword/value pairs,
   * with each keyword and its subsequent value contained in separate strings.
   */
  const MFString &getSummary() const { return _summary; }

  /**
   * Access: inputOutput.
   * @details The summary string array contains a set of keyword/value pairs,
   * with each keyword and its subsequent value contained in separate strings.
   */
  void setSummary(const MFString &value) { _summary = value; }

  void setSummary(MFString &&value) { _summary = std::move(value); }

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
  MFNode _data{};

  MFString _summary{};
};

} // namespace x3d::nodes
