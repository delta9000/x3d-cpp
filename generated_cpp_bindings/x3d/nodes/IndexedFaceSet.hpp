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
 * @class IndexedFaceSet
 * @brief IndexedFaceSet defines polygons using index lists corresponding to
 * vertex coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#IndexedFaceSet
 */
class IndexedFaceSet : public virtual X3DComposedGeometryNode {
public:
  IndexedFaceSet() = default;

  ~IndexedFaceSet() = default;

  static SFBool getDefaultConvex() { return true; }

  static SFFloat getDefaultCreaseAngle() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry3D"; }

  static int componentLevel() { return 2; }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices).
   */
  const MFInt32 &getColorIndex() const { return _colorIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColorIndexUnchecked(const MFInt32 &value) { _colorIndex = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details The convex field is a hint to renderers whether all polygons in a
   * shape are convex (true), or possibly concave (false).
   */
  const SFBool &getConvex() const { return _convex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setConvexUnchecked(const SFBool &value) { _convex = value; }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face.
   */
  const MFInt32 &getCoordIndex() const { return _coordIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCoordIndexUnchecked(const MFInt32 &value) { _coordIndex = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details creaseAngle defines angle (in radians) for determining whether
   * adjacent polygons are drawn with sharp edges or smooth shading.
   */
  const SFFloat &getCreaseAngle() const { return _creaseAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details normalIndex values define the order in which normal vectors are
   * applied to polygons (or vertices).
   */
  const MFInt32 &getNormalIndex() const { return _normalIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setNormalIndexUnchecked(const MFInt32 &value) { _normalIndex = value; }

  /**
   * MFInt32; access: inputOnly.
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices). Dispatches to the handler registered
   * via setOnSet_colorIndexHandler(); a no-op if none is set.
   */
  void onSet_colorIndex(const MFInt32 &value) {
    if (_onSet_colorIndexHandler)
      _onSet_colorIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_colorIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_colorIndexHandler = std::move(handler);
  }

  /**
   * MFInt32; access: inputOnly.
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face. Dispatches to the handler
   * registered via setOnSet_coordIndexHandler(); a no-op if none is set.
   */
  void onSet_coordIndex(const MFInt32 &value) {
    if (_onSet_coordIndexHandler)
      _onSet_coordIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_coordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_coordIndexHandler = std::move(handler);
  }

  /**
   * MFInt32; access: inputOnly.
   * @details normalIndex values define the order in which normal vectors are
   * applied to polygons (or vertices). Dispatches to the handler registered via
   * setOnSet_normalIndexHandler(); a no-op if none is set.
   */
  void onSet_normalIndex(const MFInt32 &value) {
    if (_onSet_normalIndexHandler)
      _onSet_normalIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_normalIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_normalIndexHandler = std::move(handler);
  }

  /**
   * MFInt32; access: inputOnly.
   * @details List of texture-coordinate indices mapping attached texture to
   * corresponding coordinates. Dispatches to the handler registered via
   * setOnSet_texCoordIndexHandler(); a no-op if none is set.
   */
  void onSet_texCoordIndex(const MFInt32 &value) {
    if (_onSet_texCoordIndexHandler)
      _onSet_texCoordIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_texCoordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_texCoordIndexHandler = std::move(handler);
  }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details List of texture-coordinate indices mapping attached texture to
   * corresponding coordinates.
   */
  const MFInt32 &getTexCoordIndex() const { return _texCoordIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexCoordIndexUnchecked(const MFInt32 &value) {
    _texCoordIndex = value;
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
  static void checkRangesColorIndex(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCoordIndex(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCreaseAngle(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesNormalIndex(const MFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTexCoordIndex(const MFInt32 &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  MFInt32 _colorIndex{};

  SFBool _convex{true};

  MFInt32 _coordIndex{};

  SFFloat _creaseAngle{0};

  MFInt32 _normalIndex{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_colorIndexHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_normalIndexHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_texCoordIndexHandler{};

  MFInt32 _texCoordIndex{};
};

} // namespace x3d::nodes
