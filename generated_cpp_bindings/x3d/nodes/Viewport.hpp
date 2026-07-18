// Viewport.hpp
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

#include "x3d/nodes/X3DViewportNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Viewport
 * @brief Viewport is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#Viewport
 */
class Viewport : public virtual X3DViewportNode {
public:
  /**
   * @brief Default constructor for Viewport
   */
  Viewport() = default;

  /**
   * @brief Destructor for Viewport
   */
  ~Viewport() = default;

  /**
   * @brief Get the default value for clipBoundary
   * @return MFFloat The default value
   */
  static MFFloat getDefaultClipBoundary() {
    return std::vector<float>{0, 1, 0, 1};
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "viewport"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Layering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of clipBoundary. AccessType: inputOutput
   * @details clipBoundary is specified in fractions of the normal render
   * surface in the sequence left/right/bottom/top.
   * @return MFFloat The current value of clipBoundary.
   */
  MFFloat getClipBoundary() const { return _clipBoundary; }

  /**
   * @brief Sets the value of clipBoundary. AccessType: inputOutput
   * @details clipBoundary is specified in fractions of the normal render
   * surface in the sequence left/right/bottom/top.
   * @param value The new value for clipBoundary.
   */
  void setClipBoundary(const MFFloat &value) {

    validateClipBoundary(value);

    _clipBoundary = value;
  }

  void setClipBoundary(MFFloat &&value) {

    validateClipBoundary(value);

    _clipBoundary = std::move(value);
  }

  /**
   * @brief Non-validating write of clipBoundary (runtime/reader ingest path).
   * @details Assigns without the range check setClipBoundary() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setClipBoundary() stays the
   *          enforcement point for programmatic callers.
   */
  void setClipBoundaryUnchecked(const MFFloat &value) { _clipBoundary = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Viewport").
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
  static void checkRangesClipBoundary(const MFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

private:
  static void validateClipBoundary(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("clipBoundary below minimum of 0");
      if (v > 1.0f)
        throw std::out_of_range("clipBoundary above maximum of 1");
    }
  }

  /**
   * @brief Member variable for clipBoundary.
   */

  MFFloat _clipBoundary{std::vector<float>{0, 1, 0, 1}};
};

} // namespace x3d::nodes
