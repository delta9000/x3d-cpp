// HAnimHumanoid.hpp
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
  /**
   * @brief Default constructor for HAnimHumanoid
   */
  HAnimHumanoid() = default;

  /**
   * @brief Destructor for HAnimHumanoid
   */
  ~HAnimHumanoid() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for loa
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultLoa() { return -1; }

  /**
   * @brief Get the default value for rotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for scale
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  /**
   * @brief Get the default value for scaleOrientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  /**
   * @brief Get the default value for skeletalConfiguration
   * @return SFString The default value
   */
  static SFString getDefaultSkeletalConfiguration() { return "BASIC"; }

  /**
   * @brief Get the default value for skinBindingCoords
   * @return SFNode The default value
   */
  static SFNode getDefaultSkinBindingCoords() { return nullptr; }

  /**
   * @brief Get the default value for skinBindingNormals
   * @return SFNode The default value
   */
  static SFNode getDefaultSkinBindingNormals() { return nullptr; }

  /**
   * @brief Get the default value for skinCoord
   * @return SFNode The default value
   */
  static SFNode getDefaultSkinCoord() { return nullptr; }

  /**
   * @brief Get the default value for skinNormal
   * @return SFNode The default value
   */
  static SFNode getDefaultSkinNormal() { return nullptr; }

  /**
   * @brief Get the default value for translation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for version
   * @return HanimVersionChoices The default value
   */
  static HanimVersionChoices getDefaultVersion() {
    return HanimVersionChoices::_2_0;
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

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
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

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
   * @brief Gets the value of info. AccessType: inputOutput
   * @details Contains metadata keyword=value pairs, where approved keyword
   * terms are humanoidVersion authorName authorEmail copyright creationDate
   * usageRestrictions age gender height and weight.
   * @return MFString The current value of info.
   */
  MFString getInfo() const { return _info; }

  /**
   * @brief Sets the value of info. AccessType: inputOutput
   * @details Contains metadata keyword=value pairs, where approved keyword
   * terms are humanoidVersion authorName authorEmail copyright creationDate
   * usageRestrictions age gender height and weight.
   * @param value The new value for info.
   */
  void setInfo(const MFString &value) { _info = value; }

  void setInfo(MFString &&value) { _info = std::move(value); }

  /**
   * @brief Gets the value of jointBindingPositions. AccessType: inputOutput
   * @details Specifies an array of position values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   * @return MFVec3f The current value of jointBindingPositions.
   */
  MFVec3f getJointBindingPositions() const { return _jointBindingPositions; }

  /**
   * @brief Sets the value of jointBindingPositions. AccessType: inputOutput
   * @details Specifies an array of position values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   * @param value The new value for jointBindingPositions.
   */
  void setJointBindingPositions(const MFVec3f &value) {

    _jointBindingPositions = value;
  }

  void setJointBindingPositions(MFVec3f &&value) {

    _jointBindingPositions = std::move(value);
  }

  /**
   * @brief Gets the value of jointBindingRotations. AccessType: inputOutput
   * @details Specifies an array of rotation values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   * @return MFRotation The current value of jointBindingRotations.
   */
  MFRotation getJointBindingRotations() const { return _jointBindingRotations; }

  /**
   * @brief Sets the value of jointBindingRotations. AccessType: inputOutput
   * @details Specifies an array of rotation values for each HAnimJoint node in
   * the joints field, in order, corresponding to each binding pose.
   * @param value The new value for jointBindingRotations.
   */
  void setJointBindingRotations(const MFRotation &value) {

    _jointBindingRotations = value;
  }

  void setJointBindingRotations(MFRotation &&value) {

    _jointBindingRotations = std::move(value);
  }

  /**
   * @brief Gets the value of jointBindingScales. AccessType: inputOutput
   * @details Specifies an array of scale values for each HAnimJoint node in the
   * joints field, in order, corresponding to each binding pose.
   * @return MFVec3f The current value of jointBindingScales.
   */
  MFVec3f getJointBindingScales() const { return _jointBindingScales; }

  /**
   * @brief Sets the value of jointBindingScales. AccessType: inputOutput
   * @details Specifies an array of scale values for each HAnimJoint node in the
   * joints field, in order, corresponding to each binding pose.
   * @param value The new value for jointBindingScales.
   */
  void setJointBindingScales(const MFVec3f &value) {

    _jointBindingScales = value;
  }

  void setJointBindingScales(MFVec3f &&value) {

    _jointBindingScales = std::move(value);
  }

  /**
   * @brief Gets the value of joints. AccessType: inputOutput
   * @details The joints field contains a list of USE references for all
   * HAnimJoint node instances found within the preceding skeleton hierarchy.
   * @return MFNode The current value of joints.
   */
  MFNode getJoints() const { return _joints; }

  /**
   * @brief Acceptable node types for the joints field.
   * @details Permitted X3D node types: HAnimJoint
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableJointsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint"};
    return types;
  }

  /**
   * @brief Sets the value of joints. AccessType: inputOutput
   * @details The joints field contains a list of USE references for all
   * HAnimJoint node instances found within the preceding skeleton hierarchy.
   * @param value The new value for joints.
   */
  void setJoints(const MFNode &value) { _joints = value; }

  void setJoints(MFNode &&value) { _joints = std::move(value); }

  /**
   * @brief Gets the value of loa. AccessType: inputOutput
   * @details Level Of Articulation 0.
   * @return SFInt32 The current value of loa.
   */
  SFInt32 getLoa() const { return _loa; }

  /**
   * @brief Sets the value of loa. AccessType: inputOutput
   * @details Level Of Articulation 0.
   * @param value The new value for loa.
   */
  void setLoa(const SFInt32 &value) {

    validateLoa(value);

    _loa = value;
  }

  /**
   * @brief Non-validating write of loa (runtime/reader ingest path).
   * @details Assigns without the range check setLoa() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLoa() stays the
   *          enforcement point for programmatic callers.
   */
  void setLoaUnchecked(const SFInt32 &value) { _loa = value; }

  /**
   * @brief Gets the value of motions. AccessType: inputOutput
   * @details Contains any HAnimMotion nodes that can animate the HAnimHumanoid.
   * @return MFNode The current value of motions.
   */
  MFNode getMotions() const { return _motions; }

  /**
   * @brief Acceptable node types for the motions field.
   * @details Permitted X3D node types: HAnimMotion
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMotionsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimMotion"};
    return types;
  }

  /**
   * @brief Sets the value of motions. AccessType: inputOutput
   * @details Contains any HAnimMotion nodes that can animate the HAnimHumanoid.
   * @param value The new value for motions.
   */
  void setMotions(const MFNode &value) { _motions = value; }

  void setMotions(MFNode &&value) { _motions = std::move(value); }

  /**
   * @brief Gets the value of motionsEnabled. AccessType: inputOutput
   * @details Array of boolean values corresponding to HAnimMotion nodes
   * indicating which can animate the HAnimHumanoid.
   * @return MFBool The current value of motionsEnabled.
   */
  MFBool getMotionsEnabled() const { return _motionsEnabled; }

  /**
   * @brief Sets the value of motionsEnabled. AccessType: inputOutput
   * @details Array of boolean values corresponding to HAnimMotion nodes
   * indicating which can animate the HAnimHumanoid.
   * @param value The new value for motionsEnabled.
   */
  void setMotionsEnabled(const MFBool &value) { _motionsEnabled = value; }

  void setMotionsEnabled(MFBool &&value) { _motionsEnabled = std::move(value); }

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that each HAnimHumanoid
   * node in a scene can be identified at run time for animation purposes.
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that each HAnimHumanoid
   * node in a scene can be identified at run time for animation purposes.
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief Gets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system.
   * @return SFRotation The current value of rotation.
   */
  SFRotation getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system.
   * @param value The new value for rotation.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * @brief Gets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @return SFVec3f The current value of scale.
   */
  SFVec3f getScale() const { return _scale; }

  /**
   * @brief Sets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @param value The new value for scale.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * @brief Gets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @return SFRotation The current value of scaleOrientation.
   */
  SFRotation getScaleOrientation() const { return _scaleOrientation; }

  /**
   * @brief Sets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @param value The new value for scaleOrientation.
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * @brief Gets the value of segments. AccessType: inputOutput
   * @details The segments field contains a list of USE references for all
   * HAnimSegment node instances found within the preceding skeleton hierarchy.
   * @return MFNode The current value of segments.
   */
  MFNode getSegments() const { return _segments; }

  /**
   * @brief Acceptable node types for the segments field.
   * @details Permitted X3D node types: HAnimSegment
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSegmentsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSegment"};
    return types;
  }

  /**
   * @brief Sets the value of segments. AccessType: inputOutput
   * @details The segments field contains a list of USE references for all
   * HAnimSegment node instances found within the preceding skeleton hierarchy.
   * @param value The new value for segments.
   */
  void setSegments(const MFNode &value) { _segments = value; }

  void setSegments(MFNode &&value) { _segments = std::move(value); }

  /**
   * @brief Gets the value of sites. AccessType: inputOutput
   * @details sites field contains a list of USE references for all HAnimSite
   * node instances found within the preceding skeleton hierarchy.
   * @return MFNode The current value of sites.
   */
  MFNode getSites() const { return _sites; }

  /**
   * @brief Acceptable node types for the sites field.
   * @details Permitted X3D node types: HAnimSite
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSitesNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSite"};
    return types;
  }

  /**
   * @brief Sets the value of sites. AccessType: inputOutput
   * @details sites field contains a list of USE references for all HAnimSite
   * node instances found within the preceding skeleton hierarchy.
   * @param value The new value for sites.
   */
  void setSites(const MFNode &value) { _sites = value; }

  void setSites(MFNode &&value) { _sites = std::move(value); }

  /**
   * @brief Gets the value of skeletalConfiguration. AccessType: inputOutput
   * @details Models sharing a common skeletal configuration can share
   * animations and binding poses.
   * @return SFString The current value of skeletalConfiguration.
   */
  SFString getSkeletalConfiguration() const { return _skeletalConfiguration; }

  /**
   * @brief Sets the value of skeletalConfiguration. AccessType: inputOutput
   * @details Models sharing a common skeletal configuration can share
   * animations and binding poses.
   * @param value The new value for skeletalConfiguration.
   */
  void setSkeletalConfiguration(const SFString &value) {

    _skeletalConfiguration = value;
  }

  void setSkeletalConfiguration(SFString &&value) {

    _skeletalConfiguration = std::move(value);
  }

  /**
   * @brief Gets the value of skeleton. AccessType: inputOutput
   * @details List of top-level HAnimJoint and HAnimSite nodes that create the
   * skeleton model.
   * @return MFNode The current value of skeleton.
   */
  MFNode getSkeleton() const { return _skeleton; }

  /**
   * @brief Acceptable node types for the skeleton field.
   * @details Permitted X3D node types: HAnimJoint, HAnimSite
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSkeletonNodeTypes() {
    static const std::vector<std::string> types = {"HAnimJoint", "HAnimSite"};
    return types;
  }

  /**
   * @brief Sets the value of skeleton. AccessType: inputOutput
   * @details List of top-level HAnimJoint and HAnimSite nodes that create the
   * skeleton model.
   * @param value The new value for skeleton.
   */
  void setSkeleton(const MFNode &value) { _skeleton = value; }

  void setSkeleton(MFNode &&value) { _skeleton = std::move(value); }

  /**
   * @brief Gets the value of skin. AccessType: inputOutput
   * @details List of one or more indexed mesh definitions (such as
   * IndexedFaceSet) that utilize skinCoord point and skinNormal normal data.
   * @return MFNode The current value of skin.
   */
  MFNode getSkin() const { return _skin; }

  /**
   * @brief Acceptable node types for the skin field.
   * @details Permitted X3D node types: Group, Transform, Shape, IndexedFaceSet
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSkinNodeTypes() {
    static const std::vector<std::string> types = {"Group", "Transform",
                                                   "Shape", "IndexedFaceSet"};
    return types;
  }

  /**
   * @brief Sets the value of skin. AccessType: inputOutput
   * @details List of one or more indexed mesh definitions (such as
   * IndexedFaceSet) that utilize skinCoord point and skinNormal normal data.
   * @param value The new value for skin.
   */
  void setSkin(const MFNode &value) { _skin = value; }

  void setSkin(MFNode &&value) { _skin = std::move(value); }

  /**
   * @brief Gets the value of skinBindingCoords. AccessType: inputOutput
   * @details Array of Coordinate nodes to handle non-default source pose so
   * that both skin and skeleton can be in same binding pose.
   * @return SFNode The current value of skinBindingCoords.
   */
  SFNode getSkinBindingCoords() const { return _skinBindingCoords; }

  /**
   * @brief Acceptable node types for the skinBindingCoords field.
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableSkinBindingCoordsNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * @brief Sets the value of skinBindingCoords. AccessType: inputOutput
   * @details Array of Coordinate nodes to handle non-default source pose so
   * that both skin and skeleton can be in same binding pose.
   * @param value The new value for skinBindingCoords.
   */
  void setSkinBindingCoords(const SFNode &value) { _skinBindingCoords = value; }

  void setSkinBindingCoords(SFNode &&value) {

    _skinBindingCoords = std::move(value);
  }

  /**
   * @brief Gets the value of skinBindingNormals. AccessType: inputOutput
   * @details Array of Normal nodes to handle non-default source pose so that
   * both skin and skeleton can be in same binding pose.
   * @return SFNode The current value of skinBindingNormals.
   */
  SFNode getSkinBindingNormals() const { return _skinBindingNormals; }

  /**
   * @brief Acceptable node types for the skinBindingNormals field.
   * @details Permitted X3D node types: X3DNormalNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableSkinBindingNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * @brief Sets the value of skinBindingNormals. AccessType: inputOutput
   * @details Array of Normal nodes to handle non-default source pose so that
   * both skin and skeleton can be in same binding pose.
   * @param value The new value for skinBindingNormals.
   */
  void setSkinBindingNormals(const SFNode &value) {

    _skinBindingNormals = value;
  }

  void setSkinBindingNormals(SFNode &&value) {

    _skinBindingNormals = std::move(value);
  }

  /**
   * @brief Gets the value of skinCoord. AccessType: inputOutput
   * @details Coordinate node utilized by indexed mesh definitions for skin.
   * @return SFNode The current value of skinCoord.
   */
  SFNode getSkinCoord() const { return _skinCoord; }

  /**
   * @brief Acceptable node types for the skinCoord field.
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSkinCoordNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * @brief Sets the value of skinCoord. AccessType: inputOutput
   * @details Coordinate node utilized by indexed mesh definitions for skin.
   * @param value The new value for skinCoord.
   */
  void setSkinCoord(const SFNode &value) { _skinCoord = value; }

  void setSkinCoord(SFNode &&value) { _skinCoord = std::move(value); }

  /**
   * @brief Gets the value of skinNormal. AccessType: inputOutput
   * @details Single Normal node utilized by indexed mesh definitions for skin.
   * @return SFNode The current value of skinNormal.
   */
  SFNode getSkinNormal() const { return _skinNormal; }

  /**
   * @brief Acceptable node types for the skinNormal field.
   * @details Permitted X3D node types: X3DNormalNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSkinNormalNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * @brief Sets the value of skinNormal. AccessType: inputOutput
   * @details Single Normal node utilized by indexed mesh definitions for skin.
   * @param value The new value for skinNormal.
   */
  void setSkinNormal(const SFNode &value) { _skinNormal = value; }

  void setSkinNormal(SFNode &&value) { _skinNormal = std::move(value); }

  /**
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system.
   * @return SFVec3f The current value of translation.
   */
  SFVec3f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system.
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * @brief Gets the value of version. AccessType: inputOutput
   * @details HAnimHumanoid version, where allowed value is 2.
   * @return HanimVersionChoices The current value of version.
   */
  HanimVersionChoices getVersion() const { return _version; }

  /**
   * @brief Sets the value of version. AccessType: inputOutput
   * @details HAnimHumanoid version, where allowed value is 2.
   * @param value The new value for version.
   */
  void setVersion(const HanimVersionChoices &value) { _version = value; }

  /**
   * @brief Gets the value of viewpoints. AccessType: inputOutput
   * @details List of HAnimSite nodes containing Viewpoint nodes that appear in
   * the skeleton model, usually as USE node references.
   * @return MFNode The current value of viewpoints.
   */
  MFNode getViewpoints() const { return _viewpoints; }

  /**
   * @brief Acceptable node types for the viewpoints field.
   * @details Permitted X3D node types: HAnimSite
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableViewpointsNodeTypes() {
    static const std::vector<std::string> types = {"HAnimSite"};
    return types;
  }

  /**
   * @brief Sets the value of viewpoints. AccessType: inputOutput
   * @details List of HAnimSite nodes containing Viewpoint nodes that appear in
   * the skeleton model, usually as USE node references.
   * @param value The new value for viewpoints.
   */
  void setViewpoints(const MFNode &value) { _viewpoints = value; }

  void setViewpoints(MFNode &&value) { _viewpoints = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "HAnimHumanoid").
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

  /**
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for info.
   */

  MFString _info{};

  /**
   * @brief Member variable for jointBindingPositions.
   */

  MFVec3f _jointBindingPositions{};

  /**
   * @brief Member variable for jointBindingRotations.
   */

  MFRotation _jointBindingRotations{};

  /**
   * @brief Member variable for jointBindingScales.
   */

  MFVec3f _jointBindingScales{};

  /**
   * @brief Member variable for joints.
   */

  MFNode _joints{};

  /**
   * @brief Member variable for loa.
   */

  SFInt32 _loa{-1};

  /**
   * @brief Member variable for motions.
   */

  MFNode _motions{};

  /**
   * @brief Member variable for motionsEnabled.
   */

  MFBool _motionsEnabled{};

  /**
   * @brief Member variable for name.
   */

  SFString _name{};

  /**
   * @brief Member variable for rotation.
   */

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for scale.
   */

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  /**
   * @brief Member variable for scaleOrientation.
   */

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for segments.
   */

  MFNode _segments{};

  /**
   * @brief Member variable for sites.
   */

  MFNode _sites{};

  /**
   * @brief Member variable for skeletalConfiguration.
   */

  SFString _skeletalConfiguration{"BASIC"};

  /**
   * @brief Member variable for skeleton.
   */

  MFNode _skeleton{};

  /**
   * @brief Member variable for skin.
   */

  MFNode _skin{};

  /**
   * @brief Member variable for skinBindingCoords.
   */

  SFNode _skinBindingCoords{nullptr};

  /**
   * @brief Member variable for skinBindingNormals.
   */

  SFNode _skinBindingNormals{nullptr};

  /**
   * @brief Member variable for skinCoord.
   */

  SFNode _skinCoord{nullptr};

  /**
   * @brief Member variable for skinNormal.
   */

  SFNode _skinNormal{nullptr};

  /**
   * @brief Member variable for translation.
   */

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for version.
   */

  HanimVersionChoices _version{HanimVersionChoices::_2_0};

  /**
   * @brief Member variable for viewpoints.
   */

  MFNode _viewpoints{};
};

} // namespace x3d::nodes
