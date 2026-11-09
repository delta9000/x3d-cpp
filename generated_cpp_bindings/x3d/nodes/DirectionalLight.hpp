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

#include "x3d/nodes/X3DLightNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class DirectionalLight
 * @brief DirectionalLight might not be scoped by parent Group or Transform at
 * levels 1 or 2.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#DirectionalLight
 */
class DirectionalLight : public virtual X3DLightNode {
public:
  DirectionalLight() = default;

  ~DirectionalLight() = default;

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, -1}; }

  static SFBool getDefaultGlobal() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Lighting"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Orientation vector of light relative to local coordinate system.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details Orientation vector of light relative to local coordinate system.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   */
  const SFBool &getGlobal() const { return _global; }

  /**
   * Access: inputOutput.
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   */
  void setGlobal(const SFBool &value) { _global = value; }

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
  SFVec3f _direction{SFVec3f{0, 0, -1}};

  SFBool _global{false};
};

} // namespace x3d::nodes
