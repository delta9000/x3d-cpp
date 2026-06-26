// TriangleFanSet.hpp
#ifndef TRIANGLEFANSET_HPP
#define TRIANGLEFANSET_HPP

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
 * @class TriangleFanSet
 * @brief TriangleFanSet is a geometry node containing a
 * Coordinate|CoordinateDouble node, and can also contain Color|ColorRGBA,
 * Normal and TextureCoordinate nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#TriangleFanSet
 */
class TriangleFanSet : public virtual X3DComposedGeometryNode {
public:
  /**
   * @brief Default constructor for TriangleFanSet
   */
  TriangleFanSet() = default;

  /**
   * @brief Destructor for TriangleFanSet
   */
  ~TriangleFanSet() = default;

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
   * @brief Gets the value of fanCount. AccessType: inputOutput
   * @details fanCount array provides number of vertices in each fan.
   * @return MFInt32 The current value of fanCount.
   */
  MFInt32 getFanCount() const { return _fanCount; }

  /**
   * @brief Sets the value of fanCount. AccessType: inputOutput
   * @details fanCount array provides number of vertices in each fan.
   * @param value The new value for fanCount.
   */
  void setFanCount(const MFInt32 &value) {

    validateFanCount(value);

    _fanCount = value;
  }

  void setFanCount(MFInt32 &&value) {

    validateFanCount(value);

    _fanCount = std::move(value);
  }

  /**
   * @brief Non-validating write of fanCount (runtime/reader ingest path).
   * @details Assigns without the range check setFanCount() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFanCount() stays the
   *          enforcement point for programmatic callers.
   */
  void setFanCountUnchecked(const MFInt32 &value) { _fanCount = value; }

  /**
   * @brief The X3D type name of this node (e.g. "TriangleFanSet").
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
  static void checkRangesFanCount(const MFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateFanCount(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 3)
        throw std::out_of_range("fanCount below minimum of 3");
    }
  }

  /**
   * @brief Member variable for fanCount.
   */

  MFInt32 _fanCount{};
};

#endif // TRIANGLEFANSET_HPP