// HAnimSegment.hpp
#ifndef HANIMSEGMENT_HPP
#define HANIMSEGMENT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

/**
 * @class HAnimSegment
 * @brief HAnimSegment node contains Shape geometry for each body segment,
 * providing a visual representation of the skeleton segment.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimSegment
 */
class HAnimSegment : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for HAnimSegment
   */
  HAnimSegment() = default;

  /**
   * @brief Destructor for HAnimSegment
   */
  ~HAnimSegment() = default;

  /**
   * @brief Get the default value for centerOfMass
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenterOfMass() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for coord
   * @return SFNode The default value
   */
  static SFNode getDefaultCoord() { return nullptr; }

  /**
   * @brief Get the default value for mass
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMass() { return 0; }

  /**
   * @brief Get the default value for momentsOfInertia
   * @return MFFloat The default value
   */
  static MFFloat getDefaultMomentsOfInertia() {
    return std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0};
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesHAnimSegment";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "HAnim"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of centerOfMass. AccessType: inputOutput
   * @details Location within segment of center of mass.
   * @return SFVec3f The current value of centerOfMass.
   */
  SFVec3f getCenterOfMass() const { return _centerOfMass; }

  /**
   * @brief Sets the value of centerOfMass. AccessType: inputOutput
   * @details Location within segment of center of mass.
   * @param value The new value for centerOfMass.
   */
  void setCenterOfMass(const SFVec3f &value) { _centerOfMass = value; }

  void setCenterOfMass(SFVec3f &&value) { _centerOfMass = std::move(value); }

  /**
   * @brief Gets the value of coord. AccessType: inputOutput
   * @details the coord field is used for HAnimSegment objects that have
   * deformable meshes and shall contain coordinates referenced from the
   * IndexedFaceSet for the paarent HAnimSegment object.
   * @return SFNode The current value of coord.
   */
  SFNode getCoord() const { return _coord; }

  /**
   * @brief Acceptable node types for the coord field.
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * @brief Sets the value of coord. AccessType: inputOutput
   * @details the coord field is used for HAnimSegment objects that have
   * deformable meshes and shall contain coordinates referenced from the
   * IndexedFaceSet for the paarent HAnimSegment object.
   * @param value The new value for coord.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of displacers. AccessType: inputOutput
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimSegment object.
   * @return MFNode The current value of displacers.
   */
  MFNode getDisplacers() const { return _displacers; }

  /**
   * @brief Acceptable node types for the displacers field.
   * @details Permitted X3D node types: HAnimDisplacer
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableDisplacersNodeTypes() {
    static const std::vector<std::string> types = {"HAnimDisplacer"};
    return types;
  }

  /**
   * @brief Sets the value of displacers. AccessType: inputOutput
   * @details the displacers field stores HAnimDisplacer objects for a
   * particular HAnimSegment object.
   * @param value The new value for displacers.
   */
  void setDisplacers(const MFNode &value) { _displacers = value; }

  void setDisplacers(MFNode &&value) { _displacers = std::move(value); }

  /**
   * @brief Gets the value of mass. AccessType: inputOutput
   * @details Total mass of the segment, 0 if not available, defined in mass
   * base units (default is kilograms).
   * @return SFFloat The current value of mass.
   */
  SFFloat getMass() const { return _mass; }

  /**
   * @brief Sets the value of mass. AccessType: inputOutput
   * @details Total mass of the segment, 0 if not available, defined in mass
   * base units (default is kilograms).
   * @param value The new value for mass.
   */
  void setMass(const SFFloat &value) {

    validateMass(value);

    _mass = value;
  }

  /**
   * @brief Non-validating write of mass (runtime/reader ingest path).
   * @details Assigns without the range check setMass() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMass() stays the
   *          enforcement point for programmatic callers.
   */
  void setMassUnchecked(const SFFloat &value) { _mass = value; }

  /**
   * @brief Gets the value of momentsOfInertia. AccessType: inputOutput
   * @details 3x3 moments of inertia matrix.
   * @return MFFloat The current value of momentsOfInertia.
   */
  MFFloat getMomentsOfInertia() const { return _momentsOfInertia; }

  /**
   * @brief Sets the value of momentsOfInertia. AccessType: inputOutput
   * @details 3x3 moments of inertia matrix.
   * @param value The new value for momentsOfInertia.
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
   * @brief Non-validating write of momentsOfInertia (runtime/reader ingest
   * path).
   * @details Assigns without the range check setMomentsOfInertia() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setMomentsOfInertia() stays the enforcement point for
   * programmatic callers.
   */
  void setMomentsOfInertiaUnchecked(const MFFloat &value) {
    _momentsOfInertia = value;
  }

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimSegment node
   * can be identified at run time for animation purposes.
   * @return HanimSegmentNameValues The current value of name.
   */
  HanimSegmentNameValues getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimSegment node
   * can be identified at run time for animation purposes.
   * @param value The new value for name.
   */
  void setName(const HanimSegmentNameValues &value) { _name = value; }

  /**
   * @brief The X3D type name of this node (e.g. "HAnimSegment").
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

    if (value < 0)
      throw std::out_of_range("mass below minimum of 0");
  }

  static void validateMomentsOfInertia(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0)
        throw std::out_of_range("momentsOfInertia below minimum of 0");
    }
  }

  /**
   * @brief Member variable for centerOfMass.
   */

  SFVec3f _centerOfMass{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for coord.
   */

  SFNode _coord{nullptr};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for displacers.
   */

  MFNode _displacers{};

  /**
   * @brief Member variable for mass.
   */

  SFFloat _mass{0};

  /**
   * @brief Member variable for momentsOfInertia.
   */

  MFFloat _momentsOfInertia{std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0}};

  /**
   * @brief Member variable for name.
   */

  HanimSegmentNameValues _name{};
};

#endif // HANIMSEGMENT_HPP