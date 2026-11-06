#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

#include "x3d/nodes/X3DComposedGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class IndexedQuadSet
 * @brief IndexedQuadSet is a geometry node that defines quadrilaterals.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html#IndexedQuadSet
 */
class IndexedQuadSet : public virtual X3DComposedGeometryNode {
public:
  IndexedQuadSet() = default;

  ~IndexedQuadSet() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "CADGeometry"; }

  static int componentLevel() { return 1; }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details index values provide order in which coordinates are applied.
   */
  const MFInt32 &getIndex() const { return _index; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setIndexUnchecked(const MFInt32 &value) { _index = value; }

  /**
   * MFInt32; access: inputOnly.
   * @details index values provide order in which coordinates are applied.
   *          Dispatches to the handler registered via setOnSet_indexHandler();
   *          a no-op if none is set.
   */
  void onSet_index(const MFInt32 &value) {
    if (_onSet_indexHandler)
      _onSet_indexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_indexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_indexHandler = std::move(handler);
  }

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

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesIndex(const MFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  MFInt32 _index{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_indexHandler{};
};

} // namespace x3d::nodes
