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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimDisplacer
 * @brief HAnimDisplacer nodes alter the shape of coordinate-based geometry
 * within parent HAnimJoint or HAnimSegment nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimDisplacer
 */
class HAnimDisplacer : public virtual X3DGeometricPropertyNode {
public:
  HAnimDisplacer() = default;

  ~HAnimDisplacer() = default;

  static SFFloat getDefaultWeight() { return 0.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "displacers"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 1; }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Defines index values into the parent HAnimSegment or
   * HAnimBody/HAnimHumanoid coordinate array for the mesh of vertices affected
   * by this HAnimDisplacer.
   */
  const MFInt32 &getCoordIndex() const { return _coordIndex; }

  /**
   * Access: inputOutput.
   * @details Defines index values into the parent HAnimSegment or
   * HAnimBody/HAnimHumanoid coordinate array for the mesh of vertices affected
   * by this HAnimDisplacer.
   */
  void setCoordIndex(const MFInt32 &value) { _coordIndex = value; }

  void setCoordIndex(MFInt32 &&value) { _coordIndex = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Author-provided prose that describes intended purpose of this
   * node.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Author-provided prose that describes intended purpose of this
   * node.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details displacements are a set of SFVec3f values added to neutral/resting
   * position of each of the corresponding HAnimSegment vertices (or
   * HAnimJoint/HAnimHumanoid vertices) referenced by coordIndex field.
   */
  const MFVec3f &getDisplacements() const { return _displacements; }

  /**
   * Access: inputOutput.
   * @details displacements are a set of SFVec3f values added to neutral/resting
   * position of each of the corresponding HAnimSegment vertices (or
   * HAnimJoint/HAnimHumanoid vertices) referenced by coordIndex field.
   */
  void setDisplacements(const MFVec3f &value) { _displacements = value; }

  void setDisplacements(MFVec3f &&value) { _displacements = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that HAnimDisplacer node
   * can be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that HAnimDisplacer node
   * can be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details The weigh factor has typical range [0,1] and defines the scale
   * factor applied to displacement values before adding them to neutral vertex
   * positions.
   */
  const SFFloat &getWeight() const { return _weight; }

  /**
   * Access: inputOutput.
   * @details The weigh factor has typical range [0,1] and defines the scale
   * factor applied to displacement values before adding them to neutral vertex
   * positions.
   */
  void setWeight(const SFFloat &value) { _weight = value; }

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

private:
  MFInt32 _coordIndex{};

  SFString _description{};

  MFVec3f _displacements{};

  SFString _name{};

  SFFloat _weight{0.0};
};

} // namespace x3d::nodes
