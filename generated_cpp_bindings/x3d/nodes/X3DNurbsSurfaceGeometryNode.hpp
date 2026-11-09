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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DNurbsSurfaceGeometryNode
 * @brief Abstract geometry type for all types of NURBS surfaces.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#X3DNurbsSurfaceGeometryNode
 */
class X3DNurbsSurfaceGeometryNode : public virtual X3DParametricGeometryNode {
public:
  X3DNurbsSurfaceGeometryNode() = default;

  virtual ~X3DNurbsSurfaceGeometryNode() = default;

  static SFNode getDefaultControlPoint() { return nullptr; }

  static SFBool getDefaultSolid() { return true; }

  static SFNode getDefaultTexCoord() { return nullptr; }

  static SFBool getDefaultUClosed() { return false; }

  static SFInt32 getDefaultUDimension() { return 0; }

  static SFInt32 getDefaultUOrder() { return 3; }

  static SFInt32 getDefaultUTessellation() { return 0; }

  static SFBool getDefaultVClosed() { return false; }

  static SFInt32 getDefaultVDimension() { return 0; }

  static SFInt32 getDefaultVOrder() { return 3; }

  static SFInt32 getDefaultVTessellation() { return 0; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getControlPoint() const { return _controlPoint; }

  /**
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   */
  static const std::vector<std::string> &acceptableControlPointNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setControlPoint(const SFNode &value) { _controlPoint = value; }

  void setControlPoint(SFNode &&value) { _controlPoint = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getTexCoord() const { return _texCoord; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode,
   * NurbsTextureCoordinate
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode", "NurbsTextureCoordinate"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getUClosed() const { return _uClosed; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUClosedUnchecked(const SFBool &value) { _uClosed = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getUDimension() const { return _uDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUDimensionUnchecked(const SFInt32 &value) { _uDimension = value; }

  /**
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const MFDouble &getUKnot() const { return _uKnot; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUKnotUnchecked(const MFDouble &value) { _uKnot = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getUOrder() const { return _uOrder; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUOrderUnchecked(const SFInt32 &value) { _uOrder = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFInt32 &getUTessellation() const { return _uTessellation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setUTessellation(const SFInt32 &value) { _uTessellation = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getVClosed() const { return _vClosed; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVClosedUnchecked(const SFBool &value) { _vClosed = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getVDimension() const { return _vDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVDimensionUnchecked(const SFInt32 &value) { _vDimension = value; }

  /**
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const MFDouble &getVKnot() const { return _vKnot; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVKnotUnchecked(const MFDouble &value) { _vKnot = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getVOrder() const { return _vOrder; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVOrderUnchecked(const SFInt32 &value) { _vOrder = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFInt32 &getVTessellation() const { return _vTessellation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVTessellation(const SFInt32 &value) { _vTessellation = value; }

  /**
   * MFDouble; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFDouble &getWeight() const { return _weight; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

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
  static void checkRangesUDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesUOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  SFNode _controlPoint{nullptr};

  SFBool _solid{true};

  SFNode _texCoord{nullptr};

  SFBool _uClosed{false};

  SFInt32 _uDimension{0};

  MFDouble _uKnot{};

  SFInt32 _uOrder{3};

  SFInt32 _uTessellation{0};

  SFBool _vClosed{false};

  SFInt32 _vDimension{0};

  MFDouble _vKnot{};

  SFInt32 _vOrder{3};

  SFInt32 _vTessellation{0};

  MFDouble _weight{};
};

} // namespace x3d::nodes
