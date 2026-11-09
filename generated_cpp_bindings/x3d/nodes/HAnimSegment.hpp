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

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DGroupingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimSegment
 * @brief HAnimSegment node contains Shape geometry for each body segment,
 * providing a visual representation of the skeleton segment.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimSegment
 */
class HAnimSegment : public virtual X3DGroupingNode {
public:
  HAnimSegment() = default;

  ~HAnimSegment() = default;

  static SFVec3f getDefaultCenterOfMass() { return SFVec3f{0, 0, 0}; }

  static SFNode getDefaultCoord() { return nullptr; }

  static SFFloat getDefaultMass() { return 0; }

  static MFFloat getDefaultMomentsOfInertia() {
    return std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0};
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimSegment";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Location within segment of center of mass.
   */
  const SFVec3f &getCenterOfMass() const { return _centerOfMass; }

  /**
   * Access: inputOutput.
   * @details Location within segment of center of mass.
   */
  void setCenterOfMass(const SFVec3f &value) { _centerOfMass = value; }

  void setCenterOfMass(SFVec3f &&value) { _centerOfMass = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details the coord field is used for HAnimSegment objects that have
   * deformable meshes and shall contain coordinates referenced from the
   * IndexedFaceSet for the paarent HAnimSegment object.
   */
  const SFNode &getCoord() const { return _coord; }

  /**
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details the coord field is used for HAnimSegment objects that have
   * deformable meshes and shall contain coordinates referenced from the
   * IndexedFaceSet for the paarent HAnimSegment object.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimSegment object.
   */
  const MFNode &getDisplacers() const { return _displacers; }

  /**
   * @details Permitted X3D node types: HAnimDisplacer
   */
  static const std::vector<std::string> &acceptableDisplacersNodeTypes() {
    static const std::vector<std::string> types = {"HAnimDisplacer"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimSegment object.
   */
  void setDisplacers(const MFNode &value) { _displacers = value; }

  void setDisplacers(MFNode &&value) { _displacers = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Total mass of the segment, 0 if not available, defined in mass
   * base units (default is kilograms).
   */
  const SFFloat &getMass() const { return _mass; }

  /**
   * Access: inputOutput.
   * @details Total mass of the segment, 0 if not available, defined in mass
   * base units (default is kilograms).
   */
  void setMass(const SFFloat &value) {

    validateMass(value);

    _mass = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMass() for range validation.
   */
  void setMassUnchecked(const SFFloat &value) { _mass = value; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details 3x3 moments of inertia matrix.
   */
  const MFFloat &getMomentsOfInertia() const { return _momentsOfInertia; }

  /**
   * Access: inputOutput.
   * @details 3x3 moments of inertia matrix.
   */
  void setMomentsOfInertia(const MFFloat &value) {

    validateMomentsOfInertia(value);

    _momentsOfInertia = value;
  }

  void setMomentsOfInertia(MFFloat &&value) {

    validateMomentsOfInertia(value);

    _momentsOfInertia = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMomentsOfInertia() for range validation.
   */
  void setMomentsOfInertiaUnchecked(const MFFloat &value) {
    _momentsOfInertia = value;
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that HAnimSegment node
   * can be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that HAnimSegment node
   * can be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

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
  static void checkRangesMass(const SFFloat &value, const std::string &nodeType,
                              const std::string &defName,
                              std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMomentsOfInertia(const MFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

private:
  static void validateMass(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("mass below minimum of 0");
  }

  static void validateMomentsOfInertia(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("momentsOfInertia below minimum of 0");
    }
  }

  SFVec3f _centerOfMass{SFVec3f{0, 0, 0}};

  SFNode _coord{nullptr};

  SFString _description{};

  MFNode _displacers{};

  SFFloat _mass{0};

  MFFloat _momentsOfInertia{std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0}};

  SFString _name{};
};

} // namespace x3d::nodes
