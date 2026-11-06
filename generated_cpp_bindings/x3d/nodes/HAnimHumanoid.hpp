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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimHumanoid
 * @brief The HAnimHumanoid node is used to: (a) store references to the joints,
 * segments, sites, skeleton, optional skin, and fixed viewpoints, (b) serve as
 * a container for the entire humanoid, (c) provide a convenient way of moving
 * the humanoid through its environment, and (d) store human-readable metadata
 * such as name, version, author, copyright, age, gender and other information.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimHumanoid
 */
class HAnimHumanoid : public virtual X3DChildNode,
                      public virtual X3DBoundedObject {
public:
  HAnimHumanoid() = default;

  ~HAnimHumanoid() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultLoa() { return -1; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static SFString getDefaultSkeletalConfiguration() { return "BASIC"; }

  static SFNode getDefaultSkinBindingCoords() { return nullptr; }

  static SFNode getDefaultSkinBindingNormals() { return nullptr; }

  static SFNode getDefaultSkinCoord() { return nullptr; }

  static SFNode getDefaultSkinNormal() { return nullptr; }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static HanimVersionChoices getDefaultVersion() {
    return HanimVersionChoices::_2_0;
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "HAnim"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

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
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Contains metadata keyword=value pairs, where approved keyword
   * terms are humanoidVersion authorName authorEmail copyright creationDate
   * usageRestrictions age gender height and weight.
   */
  const MFString &getInfo() const { return _info; }

  /**
   * Access: inputOutput.
   * @details Contains metadata keyword=value pairs, where approved keyword
   * terms are humanoidVersion authorName authorEmail copyright creationDate
   * usageRestrictions age gender height and weight.
   */
  void setInfo(const MFString &value) { _info = value; }

  void setInfo(MFString &&value) { _info = std::move(value); }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details Specifies an array of position values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   */
  const MFVec3f &getJointBindingPositions() const {
    return _jointBindingPositions;
  }

  /**
   * Access: inputOutput.
   * @details Specifies an array of position values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   */
  void setJointBindingPositions(const MFVec3f &value) {

    _jointBindingPositions = value;
  }

  void setJointBindingPositions(MFVec3f &&value) {

    _jointBindingPositions = std::move(value);
  }

  /**
   * MFRotation; access: inputOutput. Borrows the stored value.
   * @details Specifies an array of rotation values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   */
  const MFRotation &getJointBindingRotations() const {
    return _jointBindingRotations;
  }

  /**
   * Access: inputOutput.
   * @details Specifies an array of rotation values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   */
  void setJointBindingRotations(const MFRotation &value) {

    _jointBindingRotations = value;
  }

  void setJointBindingRotations(MFRotation &&value) {

    _jointBindingRotations = std::move(value);
  }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details Specifies an array of scale values for each HAnimJoint node in the
   * joints field, in order, corresponding to each binding pose.
   */
  const MFVec3f &getJointBindingScales() const { return _jointBindingScales; }

  /**
   * Access: inputOutput.
   * @details Specifies an array of scale values for each HAnimJoint node in the
   * joints field, in order, corresponding to each binding pose.
   */
  void setJointBindingScales(const MFVec3f &value) {

    _jointBindingScales = value;
  }

  void setJointBindingScales(MFVec3f &&value) {

    _jointBindingScales = std::move(value);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The joints field contains a list of USE references for all
   * HAnimJoint node instances found within the preceding skeleton hierarchy.
   */
  const MFNode &getJoints() const { return _joints; }

  /**
   * @details Permitted X3D node types: HAnimJoint
   */
  static const std::vector<std::string> &acceptableJointsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The joints field contains a list of USE references for all
   * HAnimJoint node instances found within the preceding skeleton hierarchy.
   */
  void setJoints(const MFNode &value) { _joints = value; }

  void setJoints(MFNode &&value) { _joints = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Level Of Articulation 0.
   */
  const SFInt32 &getLoa() const { return _loa; }

  /**
   * Access: inputOutput.
   * @details Level Of Articulation 0.
   */
  void setLoa(const SFInt32 &value) {

    validateLoa(value);

    _loa = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLoa() for range validation.
   */
  void setLoaUnchecked(const SFInt32 &value) { _loa = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Contains any HAnimMotion nodes that can animate the HAnimHumanoid.
   */
  const MFNode &getMotions() const { return _motions; }

  /**
   * @details Permitted X3D node types: HAnimMotion
   */
  static const std::vector<std::string> &acceptableMotionsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimMotion"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Contains any HAnimMotion nodes that can animate the HAnimHumanoid.
   */
  void setMotions(const MFNode &value) { _motions = value; }

  void setMotions(MFNode &&value) { _motions = std::move(value); }

  /**
   * MFBool; access: inputOutput. Borrows the stored value.
   * @details Array of boolean values corresponding to HAnimMotion nodes
   * indicating which can animate the HAnimHumanoid.
   */
  const MFBool &getMotionsEnabled() const { return _motionsEnabled; }

  /**
   * Access: inputOutput.
   * @details Array of boolean values corresponding to HAnimMotion nodes
   * indicating which can animate the HAnimHumanoid.
   */
  void setMotionsEnabled(const MFBool &value) { _motionsEnabled = value; }

  void setMotionsEnabled(MFBool &&value) { _motionsEnabled = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Unique name attribute must be defined so that each HAnimHumanoid
   * node in a scene can be identified at run time for animation purposes.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Unique name attribute must be defined so that each HAnimHumanoid
   * node in a scene can be identified at run time for animation purposes.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation of children relative to local coordinate system.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details Orientation of children relative to local coordinate system.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  const SFVec3f &getScale() const { return _scale; }

  /**
   * Access: inputOutput.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  const SFRotation &getScaleOrientation() const { return _scaleOrientation; }

  /**
   * Access: inputOutput.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The segments field contains a list of USE references for all
   * HAnimSegment node instances found within the preceding skeleton hierarchy.
   */
  const MFNode &getSegments() const { return _segments; }

  /**
   * @details Permitted X3D node types: HAnimSegment
   */
  static const std::vector<std::string> &acceptableSegmentsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSegment"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The segments field contains a list of USE references for all
   * HAnimSegment node instances found within the preceding skeleton hierarchy.
   */
  void setSegments(const MFNode &value) { _segments = value; }

  void setSegments(MFNode &&value) { _segments = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details sites field contains a list of USE references for all HAnimSite
   * node instances found within the preceding skeleton hierarchy.
   */
  const MFNode &getSites() const { return _sites; }

  /**
   * @details Permitted X3D node types: HAnimSite
   */
  static const std::vector<std::string> &acceptableSitesNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSite"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details sites field contains a list of USE references for all HAnimSite
   * node instances found within the preceding skeleton hierarchy.
   */
  void setSites(const MFNode &value) { _sites = value; }

  void setSites(MFNode &&value) { _sites = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Models sharing a common skeletal configuration can share
   * animations and binding poses.
   */
  const SFString &getSkeletalConfiguration() const {
    return _skeletalConfiguration;
  }

  /**
   * Access: inputOutput.
   * @details Models sharing a common skeletal configuration can share
   * animations and binding poses.
   */
  void setSkeletalConfiguration(const SFString &value) {

    _skeletalConfiguration = value;
  }

  void setSkeletalConfiguration(SFString &&value) {

    _skeletalConfiguration = std::move(value);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details List of top-level HAnimJoint and HAnimSite nodes that create the
   * skeleton model.
   */
  const MFNode &getSkeleton() const { return _skeleton; }

  /**
   * @details Permitted X3D node types: HAnimJoint, HAnimSite
   */
  static const std::vector<std::string> &acceptableSkeletonNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint", "HAnimSite"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details List of top-level HAnimJoint and HAnimSite nodes that create the
   * skeleton model.
   */
  void setSkeleton(const MFNode &value) { _skeleton = value; }

  void setSkeleton(MFNode &&value) { _skeleton = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details List of one or more indexed mesh definitions (such as
   * IndexedFaceSet) that utilize skinCoord point and skinNormal normal data.
   */
  const MFNode &getSkin() const { return _skin; }

  /**
   * @details Permitted X3D node types: Group, Transform, Shape, IndexedFaceSet
   */
  static const std::vector<std::string> &acceptableSkinNodeTypes() {
    static const std::vector<std::string> types = {"Group", "Transform",
                                                   "Shape", "IndexedFaceSet"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details List of one or more indexed mesh definitions (such as
   * IndexedFaceSet) that utilize skinCoord point and skinNormal normal data.
   */
  void setSkin(const MFNode &value) { _skin = value; }

  void setSkin(MFNode &&value) { _skin = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Array of Coordinate nodes to handle non-default source pose so
   * that both skin and skeleton can be in same binding pose.
   */
  const SFNode &getSkinBindingCoords() const { return _skinBindingCoords; }

  /**
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   */
  static const std::vector<std::string> &
  acceptableSkinBindingCoordsNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Array of Coordinate nodes to handle non-default source pose so
   * that both skin and skeleton can be in same binding pose.
   */
  void setSkinBindingCoords(const SFNode &value) { _skinBindingCoords = value; }

  void setSkinBindingCoords(SFNode &&value) {

    _skinBindingCoords = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Array of Normal nodes to handle non-default source pose so that
   * both skin and skeleton can be in same binding pose.
   */
  const SFNode &getSkinBindingNormals() const { return _skinBindingNormals; }

  /**
   * @details Permitted X3D node types: X3DNormalNode
   */
  static const std::vector<std::string> &
  acceptableSkinBindingNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Array of Normal nodes to handle non-default source pose so that
   * both skin and skeleton can be in same binding pose.
   */
  void setSkinBindingNormals(const SFNode &value) {

    _skinBindingNormals = value;
  }

  void setSkinBindingNormals(SFNode &&value) {

    _skinBindingNormals = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Coordinate node utilized by indexed mesh definitions for skin.
   */
  const SFNode &getSkinCoord() const { return _skinCoord; }

  /**
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   */
  static const std::vector<std::string> &acceptableSkinCoordNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Coordinate node utilized by indexed mesh definitions for skin.
   */
  void setSkinCoord(const SFNode &value) { _skinCoord = value; }

  void setSkinCoord(SFNode &&value) { _skinCoord = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single Normal node utilized by indexed mesh definitions for skin.
   */
  const SFNode &getSkinNormal() const { return _skinNormal; }

  /**
   * @details Permitted X3D node types: X3DNormalNode
   */
  static const std::vector<std::string> &acceptableSkinNormalNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single Normal node utilized by indexed mesh definitions for skin.
   */
  void setSkinNormal(const SFNode &value) { _skinNormal = value; }

  void setSkinNormal(SFNode &&value) { _skinNormal = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position of children relative to local coordinate system.
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details Position of children relative to local coordinate system.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * HanimVersionChoices; access: inputOutput. Borrows the stored value.
   * @details HAnimHumanoid version, where allowed value is 2.
   */
  const HanimVersionChoices &getVersion() const { return _version; }

  /**
   * Access: inputOutput.
   * @details HAnimHumanoid version, where allowed value is 2.
   */
  void setVersion(const HanimVersionChoices &value) { _version = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details List of HAnimSite nodes containing Viewpoint nodes that appear in
   * the skeleton model, usually as USE node references.
   */
  const MFNode &getViewpoints() const { return _viewpoints; }

  /**
   * @details Permitted X3D node types: HAnimSite
   */
  static const std::vector<std::string> &acceptableViewpointsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSite"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details List of HAnimSite nodes containing Viewpoint nodes that appear in
   * the skeleton model, usually as USE node references.
   */
  void setViewpoints(const MFNode &value) { _viewpoints = value; }

  void setViewpoints(MFNode &&value) { _viewpoints = std::move(value); }

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
  static void checkRangesLoa(const SFInt32 &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out);

private:
  static void validateLoa(const SFInt32 &value) {

    if (value < -1)
      throw std::out_of_range("loa below minimum of -1");
    if (value > 4)
      throw std::out_of_range("loa above maximum of 4");
  }

  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFString _description{};

  MFString _info{};

  MFVec3f _jointBindingPositions{};

  MFRotation _jointBindingRotations{};

  MFVec3f _jointBindingScales{};

  MFNode _joints{};

  SFInt32 _loa{-1};

  MFNode _motions{};

  MFBool _motionsEnabled{};

  SFString _name{};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  MFNode _segments{};

  MFNode _sites{};

  SFString _skeletalConfiguration{"BASIC"};

  MFNode _skeleton{};

  MFNode _skin{};

  SFNode _skinBindingCoords{nullptr};

  SFNode _skinBindingNormals{nullptr};

  SFNode _skinCoord{nullptr};

  SFNode _skinNormal{nullptr};

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  HanimVersionChoices _version{HanimVersionChoices::_2_0};

  MFNode _viewpoints{};
};

} // namespace x3d::nodes
