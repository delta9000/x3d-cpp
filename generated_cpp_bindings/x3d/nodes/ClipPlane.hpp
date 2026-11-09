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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ClipPlane
 * @brief ClipPlane specifies a single plane equation used to clip (i.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#ClipPlane
 */
class ClipPlane : public virtual X3DChildNode {
public:
  ClipPlane() = default;

  ~ClipPlane() = default;

  static SFBool getDefaultEnabled() { return true; }

  static SFVec4f getDefaultPlane() { return SFVec4f{0, 1, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 5; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables node operation.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables node operation.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFVec4f; access: inputOutput. Borrows the stored value.
   * @details If (a,b,c,d) is the plane, with the first three components being a
   * normalized vector describing the plane's normal direction (and thus the
   * fourth component d being distance from the origin), a point (x,y,z) is
   * visible to the user, with regards to the clipping plane, if a*x+b*y+c*z+d
   * is greater than 0.
   */
  const SFVec4f &getPlane() const { return _plane; }

  /**
   * Access: inputOutput.
   * @details If (a,b,c,d) is the plane, with the first three components being a
   * normalized vector describing the plane's normal direction (and thus the
   * fourth component d being distance from the origin), a point (x,y,z) is
   * visible to the user, with regards to the clipping plane, if a*x+b*y+c*z+d
   * is greater than 0.
   */
  void setPlane(const SFVec4f &value) {

    validatePlane(value);

    _plane = value;
  }

  void setPlane(SFVec4f &&value) {

    validatePlane(value);

    _plane = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setPlane() for range validation.
   */
  void setPlaneUnchecked(const SFVec4f &value) { _plane = value; }

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
  static void checkRangesPlane(const SFVec4f &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validatePlane(const SFVec4f &value) {

    if (value.x < -1.0f)
      throw std::out_of_range("plane.x below minimum of -1");
    if (value.x > 1.0f)
      throw std::out_of_range("plane.x above maximum of 1");

    if (value.y < -1.0f)
      throw std::out_of_range("plane.y below minimum of -1");
    if (value.y > 1.0f)
      throw std::out_of_range("plane.y above maximum of 1");

    if (value.z < -1.0f)
      throw std::out_of_range("plane.z below minimum of -1");
    if (value.z > 1.0f)
      throw std::out_of_range("plane.z above maximum of 1");

    if (value.w < -1.0f)
      throw std::out_of_range("plane.w below minimum of -1");
    if (value.w > 1.0f)
      throw std::out_of_range("plane.w above maximum of 1");
  }

  SFBool _enabled{true};

  SFVec4f _plane{SFVec4f{0, 1, 0, 0}};
};

} // namespace x3d::nodes
