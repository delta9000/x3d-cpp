// GeoMetadata.hpp
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
  /**
   * @brief Default constructor for GeoMetadata
   */
  GeoMetadata() = default;

  /**
   * @brief Destructor for GeoMetadata
   */
  ~GeoMetadata() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Geospatial"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of data. AccessType: inputOutput
   * @details DEF list of all nodes that implement this data.
   * @return MFNode The current value of data.
   */
  MFNode getData() const { return _data; }

  /**
   * @brief Acceptable node types for the data field.
   * @details Permitted X3D node types: X3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableDataNodeTypes() {
    static const std::vector<std::string> types = {"X3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of data. AccessType: inputOutput
   * @details DEF list of all nodes that implement this data.
   * @param value The new value for data.
   */
  void setData(const MFNode &value) { _data = value; }

  void setData(MFNode &&value) { _data = std::move(value); }

  /**
   * @brief Gets the value of summary. AccessType: inputOutput
   * @details The summary string array contains a set of keyword/value pairs,
   * with each keyword and its subsequent value contained in separate strings.
   * @return MFString The current value of summary.
   */
  MFString getSummary() const { return _summary; }

  /**
   * @brief Sets the value of summary. AccessType: inputOutput
   * @details The summary string array contains a set of keyword/value pairs,
   * with each keyword and its subsequent value contained in separate strings.
   * @param value The new value for summary.
   */
  void setSummary(const MFString &value) { _summary = value; }

  void setSummary(MFString &&value) { _summary = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "GeoMetadata").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  /**
   * @brief Member variable for data.
   */

  MFNode _data{};

  /**
   * @brief Member variable for summary.
   */

  MFString _summary{};
};

} // namespace x3d::nodes
