// TriangleStripSet.hpp
#ifndef TRIANGLESTRIPSET_HPP
#define TRIANGLESTRIPSET_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

#include "X3DComposedGeometryNode.hpp"

/**
 * @class TriangleStripSet
 * @brief TriangleStripSet is a geometry node containing a
 * Coordinate|CoordinateDouble node, and can also contain Color|ColorRGBA,
 * Normal and TextureCoordinate nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#TriangleStripSet
 */
class TriangleStripSet : public virtual X3DComposedGeometryNode {
public:
  /**
   * @brief Default constructor for TriangleStripSet
   */
  TriangleStripSet() = default;

  /**
   * @brief Destructor for TriangleStripSet
   */
  ~TriangleStripSet() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of stripCount. AccessType: inputOutput
   * @details stripCount array provides number of vertices in each strip.
   * @return MFInt32 The current value of stripCount.
   */
  MFInt32 getStripCount() const { return _stripCount; }

  /**
   * @brief Sets the value of stripCount. AccessType: inputOutput
   * @details stripCount array provides number of vertices in each strip.
   * @param value The new value for stripCount.
   */
  void setStripCount(const MFInt32 &value) {

    validateStripCount(value);

    _stripCount = value;
  }

  void setStripCount(MFInt32 &&value) {

    validateStripCount(value);

    _stripCount = std::move(value);
  }

  /**
   * @brief Non-validating write of stripCount (runtime/reader ingest path).
   * @details Assigns without the range check setStripCount() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setStripCount() stays the
   *          enforcement point for programmatic callers.
   */
  void setStripCountUnchecked(const MFInt32 &value) { _stripCount = value; }

  /**
   * @brief The X3D type name of this node (e.g. "TriangleStripSet").
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
  static void checkRangesStripCount(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateStripCount(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 3)
        throw std::out_of_range("stripCount below minimum of 3");
    }
  }

  /**
   * @brief Member variable for stripCount.
   */

  MFInt32 _stripCount{};
};

#endif // TRIANGLESTRIPSET_HPP