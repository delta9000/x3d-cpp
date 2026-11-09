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

#include "x3d/nodes/X3DParametricGeometryNode.hpp"

#include "x3d/nodes/X3DNurbsSurfaceGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsTrimmedSurface
 * @brief NurbsTrimmedSurface generates texture coordinates from a Non-Uniform
 * Rational B-Spline (NURBS) surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsTrimmedSurface
 */
class NurbsTrimmedSurface : public virtual X3DNurbsSurfaceGeometryNode {
public:
  NurbsTrimmedSurface() = default;

  ~NurbsTrimmedSurface() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 4; }

  /**
   * @details Permitted X3D node types: Contour2D
   */
  static const std::vector<std::string> &
  acceptableAddTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnAddTrimmingContourHandler(); a no-op if none is set.
   */
  void onAddTrimmingContour(const MFNode &value) {
    if (_onAddTrimmingContourHandler)
      _onAddTrimmingContourHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnAddTrimmingContourHandler(std::function<void(const MFNode &)> handler) {
    _onAddTrimmingContourHandler = std::move(handler);
  }

  /**
   * @details Permitted X3D node types: Contour2D
   */
  static const std::vector<std::string> &
  acceptableRemoveTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveTrimmingContourHandler(); a no-op if none is set.
   */
  void onRemoveTrimmingContour(const MFNode &value) {
    if (_onRemoveTrimmingContourHandler)
      _onRemoveTrimmingContourHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnRemoveTrimmingContourHandler(
      std::function<void(const MFNode &)> handler) {
    _onRemoveTrimmingContourHandler = std::move(handler);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details A set of Contour2D nodes are used as trimming loops.
   */
  const MFNode &getTrimmingContour() const { return _trimmingContour; }

  /**
   * @details Permitted X3D node types: Contour2D
   */
  static const std::vector<std::string> &acceptableTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details A set of Contour2D nodes are used as trimming loops.
   */
  void setTrimmingContour(const MFNode &value) { _trimmingContour = value; }

  void setTrimmingContour(MFNode &&value) {

    _trimmingContour = std::move(value);
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

private:
  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onAddTrimmingContourHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveTrimmingContourHandler{};

  MFNode _trimmingContour{};
};

} // namespace x3d::nodes
