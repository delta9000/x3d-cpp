// Normal.hpp
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

#include "x3d/nodes/X3DNormalNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Normal
 * @brief Normal defines a set of 3D surface-normal vectors that apply either to
 * a sibling Coordinate|CoordinateDouble node, or else to a parent ElevationGrid
 * node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#Normal
 */
class Normal : public virtual X3DNormalNode {
public:
  /**
   * @brief Default constructor for Normal
   */
  Normal() = default;

  /**
   * @brief Destructor for Normal
   */
  ~Normal() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DNormalNode";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "normal"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of vector. AccessType: inputOutput
   * @details set of unit-length normal vectors, corresponding to indexed
   * polygons or vertices.
   * @return MFVec3f The current value of vector.
   */
  MFVec3f getVector() const { return _vector; }

  /**
   * @brief Sets the value of vector. AccessType: inputOutput
   * @details set of unit-length normal vectors, corresponding to indexed
   * polygons or vertices.
   * @param value The new value for vector.
   */
  void setVector(const MFVec3f &value) {

    validateVector(value);

    _vector = value;
  }

  void setVector(MFVec3f &&value) {

    validateVector(value);

    _vector = std::move(value);
  }

  /**
   * @brief Non-validating write of vector (runtime/reader ingest path).
   * @details Assigns without the range check setVector() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setVector() stays the
   *          enforcement point for programmatic callers.
   */
  void setVectorUnchecked(const MFVec3f &value) { _vector = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Normal").
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
  static void checkRangesVector(const MFVec3f &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateVector(const MFVec3f &value) {

    for (const auto &v : value) {

      if (v.x < -1.0f)
        throw std::out_of_range("vector.x below minimum of -1");
      if (v.x > 1.0f)
        throw std::out_of_range("vector.x above maximum of 1");

      if (v.y < -1.0f)
        throw std::out_of_range("vector.y below minimum of -1");
      if (v.y > 1.0f)
        throw std::out_of_range("vector.y above maximum of 1");

      if (v.z < -1.0f)
        throw std::out_of_range("vector.z below minimum of -1");
      if (v.z > 1.0f)
        throw std::out_of_range("vector.z above maximum of 1");
    }
  }

  /**
   * @brief Member variable for vector.
   */

  MFVec3f _vector{};
};

} // namespace x3d::nodes
