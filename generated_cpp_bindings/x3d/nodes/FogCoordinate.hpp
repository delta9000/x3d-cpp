// FogCoordinate.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class FogCoordinate
 * @brief FogCoordinate defines a set of explicit fog depths on a per-vertex
 * basis, overriding Fog visibilityRange.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#FogCoordinate
 */
class FogCoordinate : public virtual X3DGeometricPropertyNode {
public:
  /**
   * @brief Default constructor for FogCoordinate
   */
  FogCoordinate() = default;

  /**
   * @brief Destructor for FogCoordinate
   */
  ~FogCoordinate() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "fogCoord"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "EnvironmentalEffects"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of depth. AccessType: inputOutput
   * @details depth contains a set of 3D coordinate (triplet) point values.
   * @return MFFloat The current value of depth.
   */
  MFFloat getDepth() const { return _depth; }

  /**
   * @brief Sets the value of depth. AccessType: inputOutput
   * @details depth contains a set of 3D coordinate (triplet) point values.
   * @param value The new value for depth.
   */
  void setDepth(const MFFloat &value) {

    validateDepth(value);

    _depth = value;
  }

  void setDepth(MFFloat &&value) {

    validateDepth(value);

    _depth = std::move(value);
  }

  /**
   * @brief Non-validating write of depth (runtime/reader ingest path).
   * @details Assigns without the range check setDepth() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDepth() stays the
   *          enforcement point for programmatic callers.
   */
  void setDepthUnchecked(const MFFloat &value) { _depth = value; }

  /**
   * @brief The X3D type name of this node (e.g. "FogCoordinate").
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

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDepth(const MFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateDepth(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0)
        throw std::out_of_range("depth below minimum of 0");
      if (v > 1)
        throw std::out_of_range("depth above maximum of 1");
    }
  }

  /**
   * @brief Member variable for depth.
   */

  MFFloat _depth{};
};

} // namespace x3d::nodes
