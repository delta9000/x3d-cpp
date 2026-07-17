// ParticleSystem.hpp
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

#include "x3d/nodes/X3DShapeNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ParticleSystem
 * @brief ParticleSystem specifies a complete particle system.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#ParticleSystem
 */
class ParticleSystem : public virtual X3DShapeNode {
public:
  /**
   * @brief Default constructor for ParticleSystem
   */
  ParticleSystem() = default;

  /**
   * @brief Destructor for ParticleSystem
   */
  ~ParticleSystem() = default;

  /**
   * @brief Get the default value for color
   * @return SFNode The default value
   */
  static SFNode getDefaultColor() { return nullptr; }

  /**
   * @brief Get the default value for createParticles
   * @return SFBool The default value
   */
  static SFBool getDefaultCreateParticles() { return true; }

  /**
   * @brief Get the default value for emitter
   * @return SFNode The default value
   */
  static SFNode getDefaultEmitter() { return nullptr; }

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for geometry
   * @return SFNode The default value
   */
  static SFNode getDefaultGeometry() { return nullptr; }

  /**
   * @brief Get the default value for geometryType
   * @return SFString The default value
   */
  static SFString getDefaultGeometryType() { return "QUAD"; }

  /**
   * @brief Get the default value for lifetimeVariation
   * @return SFFloat The default value
   */
  static SFFloat getDefaultLifetimeVariation() { return 0.25; }

  /**
   * @brief Get the default value for maxParticles
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMaxParticles() { return 200; }

  /**
   * @brief Get the default value for particleLifetime
   * @return SFFloat The default value
   */
  static SFFloat getDefaultParticleLifetime() { return 5; }

  /**
   * @brief Get the default value for particleSize
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultParticleSize() { return SFVec2f{0.02, 0.02}; }

  /**
   * @brief Get the default value for texCoord
   * @return SFNode The default value
   */
  static SFNode getDefaultTexCoord() { return nullptr; }

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
  static std::string componentName() { return "ParticleSystems"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of color. AccessType: initializeOnly
   * @details The color field contains Color|ColorRGBA nodes as a series of
   * color values to be used at the given colorKey points in time.
   * @return SFNode The current value of color.
   */
  SFNode getColor() const { return _color; }

  /**
   * @brief Acceptable node types for the color field.
   * @details Permitted X3D node types: X3DColorNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableColorNodeTypes() {
    static const std::vector<std::string> types = {"X3DColorNode"};
    return types;
  }
  /**
   * @brief Data-layer write of color (reader/init ingest path).
   * @details color is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setColor().
   */
  void setColorUnchecked(const SFNode &value) { _color = value; }

  /**
   * @brief Gets the value of colorKey. AccessType: initializeOnly
   * @details Array of time intervals in seconds, corresponding to particle
   * lifetime, that are used to interpolate color array values.
   * @return MFFloat The current value of colorKey.
   */
  MFFloat getColorKey() const { return _colorKey; }
  /**
   * @brief Data-layer write of colorKey (reader/init ingest path).
   * @details colorKey is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setColorKey().
   */
  void setColorKeyUnchecked(const MFFloat &value) { _colorKey = value; }

  /**
   * @brief Gets the value of createParticles. AccessType: inputOutput
   * @details Enables/disables creation of new particles, while any existing
   * particles remain in existence and continue to animate until the end of
   * their lifetimes.
   * @return SFBool The current value of createParticles.
   */
  SFBool getCreateParticles() const { return _createParticles; }

  /**
   * @brief Sets the value of createParticles. AccessType: inputOutput
   * @details Enables/disables creation of new particles, while any existing
   * particles remain in existence and continue to animate until the end of
   * their lifetimes.
   * @param value The new value for createParticles.
   */
  void setCreateParticles(const SFBool &value) { _createParticles = value; }

  /**
   * @brief Gets the value of emitter. AccessType: initializeOnly
   * @details The emitter field specifies the type of emitter geometry and
   * properties that the particles are given for their initial positions.
   * @return SFNode The current value of emitter.
   */
  SFNode getEmitter() const { return _emitter; }

  /**
   * @brief Acceptable node types for the emitter field.
   * @details Permitted X3D node types: X3DParticleEmitterNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableEmitterNodeTypes() {
    static const std::vector<std::string> types = {"X3DParticleEmitterNode"};
    return types;
  }
  /**
   * @brief Data-layer write of emitter (reader/init ingest path).
   * @details emitter is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setEmitter().
   */
  void setEmitterUnchecked(const SFNode &value) { _emitter = value; }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of geometry. AccessType: inputOutput
   * @details Single contained geometry node provides geometry used for each
   * particle when geometryType=GEOMETRY.
   * @return SFNode The current value of geometry.
   */
  SFNode getGeometry() const { return _geometry; }

  /**
   * @brief Acceptable node types for the geometry field.
   * @details Permitted X3D node types: X3DGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry. AccessType: inputOutput
   * @details Single contained geometry node provides geometry used for each
   * particle when geometryType=GEOMETRY.
   * @param value The new value for geometry.
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

  /**
   * @brief Gets the value of geometryType. AccessType: initializeOnly
   * @details specifies type of geometry used to represent individual particles.
   * @return SFString The current value of geometryType.
   */
  SFString getGeometryType() const { return _geometryType; }
  /**
   * @brief Data-layer write of geometryType (reader/init ingest path).
   * @details geometryType is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeometryType().
   */
  void setGeometryTypeUnchecked(const SFString &value) {
    _geometryType = value;
  }

  /**
   * @brief Gets the value of isActive. AccessType: outputOnly
   * @details isActive true/false events are sent when playback starts/stops.
   * @return SFBool The current value of isActive.
   */
  SFBool getIsActive() const { return _isActive; }

  /**
   * @brief Emit an output value on isActive. AccessType: outputOnly
   * @details isActive true/false events are sent when playback starts/stops.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * @brief Gets the value of lifetimeVariation. AccessType: inputOutput
   * @details TODO not properly defined in X3D spedification.
   * @return SFFloat The current value of lifetimeVariation.
   */
  SFFloat getLifetimeVariation() const { return _lifetimeVariation; }

  /**
   * @brief Sets the value of lifetimeVariation. AccessType: inputOutput
   * @details TODO not properly defined in X3D spedification.
   * @param value The new value for lifetimeVariation.
   */
  void setLifetimeVariation(const SFFloat &value) {

    validateLifetimeVariation(value);

    _lifetimeVariation = value;
  }

  /**
   * @brief Non-validating write of lifetimeVariation (runtime/reader ingest
   * path).
   * @details Assigns without the range check setLifetimeVariation() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setLifetimeVariation() stays the
   *          enforcement point for programmatic callers.
   */
  void setLifetimeVariationUnchecked(const SFFloat &value) {
    _lifetimeVariation = value;
  }

  /**
   * @brief Gets the value of maxParticles. AccessType: inputOutput
   * @details Maximum number of particles to be generated at one time (subject
   * to player limitations).
   * @return SFInt32 The current value of maxParticles.
   */
  SFInt32 getMaxParticles() const { return _maxParticles; }

  /**
   * @brief Sets the value of maxParticles. AccessType: inputOutput
   * @details Maximum number of particles to be generated at one time (subject
   * to player limitations).
   * @param value The new value for maxParticles.
   */
  void setMaxParticles(const SFInt32 &value) {

    validateMaxParticles(value);

    _maxParticles = value;
  }

  /**
   * @brief Non-validating write of maxParticles (runtime/reader ingest path).
   * @details Assigns without the range check setMaxParticles() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMaxParticles() stays the
   *          enforcement point for programmatic callers.
   */
  void setMaxParticlesUnchecked(const SFInt32 &value) { _maxParticles = value; }

  /**
   * @brief Gets the value of particleLifetime. AccessType: inputOutput
   * @details TODO not properly defined in X3D spedification.
   * @return SFFloat The current value of particleLifetime.
   */
  SFFloat getParticleLifetime() const { return _particleLifetime; }

  /**
   * @brief Sets the value of particleLifetime. AccessType: inputOutput
   * @details TODO not properly defined in X3D spedification.
   * @param value The new value for particleLifetime.
   */
  void setParticleLifetime(const SFFloat &value) {

    validateParticleLifetime(value);

    _particleLifetime = value;
  }

  /**
   * @brief Non-validating write of particleLifetime (runtime/reader ingest
   * path).
   * @details Assigns without the range check setParticleLifetime() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setParticleLifetime() stays the enforcement point for
   * programmatic callers.
   */
  void setParticleLifetimeUnchecked(const SFFloat &value) {
    _particleLifetime = value;
  }

  /**
   * @brief Gets the value of particleSize. AccessType: inputOutput
   * @details particleSize describes width and height dimensions for each
   * particle in length base units (default is meters).
   * @return SFVec2f The current value of particleSize.
   */
  SFVec2f getParticleSize() const { return _particleSize; }

  /**
   * @brief Sets the value of particleSize. AccessType: inputOutput
   * @details particleSize describes width and height dimensions for each
   * particle in length base units (default is meters).
   * @param value The new value for particleSize.
   */
  void setParticleSize(const SFVec2f &value) {

    validateParticleSize(value);

    _particleSize = value;
  }

  void setParticleSize(SFVec2f &&value) {

    validateParticleSize(value);

    _particleSize = std::move(value);
  }

  /**
   * @brief Non-validating write of particleSize (runtime/reader ingest path).
   * @details Assigns without the range check setParticleSize() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setParticleSize() stays the
   *          enforcement point for programmatic callers.
   */
  void setParticleSizeUnchecked(const SFVec2f &value) { _particleSize = value; }

  /**
   * @brief Gets the value of physics. AccessType: initializeOnly
   * @details After being created, the individual particles are then manipulated
   * according to the physics model(s) specified in the physics field.
   * @return const MFNode& The current value of physics.
   */
  const MFNode &getPhysics() const { return _physics; }

  /**
   * @brief Acceptable node types for the physics field.
   * @details Permitted X3D node types: X3DParticlePhysicsModelNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptablePhysicsNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DParticlePhysicsModelNode"};
    return types;
  }
  /**
   * @brief Data-layer write of physics (reader/init ingest path).
   * @details physics is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setPhysics().
   */
  void setPhysicsUnchecked(const MFNode &value) { _physics = value; }

  /**
   * @brief Gets the value of texCoord. AccessType: initializeOnly
   * @details texture coordinates of the provided texture(s) in the Appearance
   * node, over time.
   * @return SFNode The current value of texCoord.
   */
  SFNode getTexCoord() const { return _texCoord; }

  /**
   * @brief Acceptable node types for the texCoord field.
   * @details Permitted X3D node types: TextureCoordinate,
   * TextureCoordinateGenerator
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "TextureCoordinate", "TextureCoordinateGenerator"};
    return types;
  }
  /**
   * @brief Data-layer write of texCoord (reader/init ingest path).
   * @details texCoord is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setTexCoord().
   */
  void setTexCoordUnchecked(const SFNode &value) { _texCoord = value; }

  /**
   * @brief Gets the value of texCoordKey. AccessType: initializeOnly
   * @details Array of time intervals in seconds, corresponding to particle
   * lifetime, that are used to sequence texCoord array values.
   * @return MFFloat The current value of texCoordKey.
   */
  MFFloat getTexCoordKey() const { return _texCoordKey; }
  /**
   * @brief Data-layer write of texCoordKey (reader/init ingest path).
   * @details texCoordKey is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setTexCoordKey().
   */
  void setTexCoordKeyUnchecked(const MFFloat &value) { _texCoordKey = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ParticleSystem").
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
  static void checkRangesColorKey(const MFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesLifetimeVariation(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMaxParticles(const SFInt32 &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesParticleLifetime(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesParticleSize(const SFVec2f &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTexCoordKey(const MFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateLifetimeVariation(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("lifetimeVariation below minimum of 0");
    if (value > 1)
      throw std::out_of_range("lifetimeVariation above maximum of 1");
  }

  static void validateMaxParticles(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("maxParticles below minimum of 0");
  }

  static void validateParticleLifetime(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("particleLifetime below minimum of 0");
  }

  static void validateParticleSize(const SFVec2f &value) {

    if (value.x < 0)
      throw std::out_of_range("particleSize.x below minimum of 0");

    if (value.y < 0)
      throw std::out_of_range("particleSize.y below minimum of 0");
  }

  /**
   * @brief Member variable for color.
   */

  SFNode _color{nullptr};

  /**
   * @brief Member variable for colorKey.
   */

  MFFloat _colorKey{};

  /**
   * @brief Member variable for createParticles.
   */

  SFBool _createParticles{true};

  /**
   * @brief Member variable for emitter.
   */

  SFNode _emitter{nullptr};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for geometry.
   */

  SFNode _geometry{nullptr};

  /**
   * @brief Member variable for geometryType.
   */

  SFString _geometryType{"QUAD"};

  /**
   * @brief Member variable for isActive.
   */

  SFBool _isActive{};

  /**
   * @brief Member variable for lifetimeVariation.
   */

  SFFloat _lifetimeVariation{0.25};

  /**
   * @brief Member variable for maxParticles.
   */

  SFInt32 _maxParticles{200};

  /**
   * @brief Member variable for particleLifetime.
   */

  SFFloat _particleLifetime{5};

  /**
   * @brief Member variable for particleSize.
   */

  SFVec2f _particleSize{SFVec2f{0.02, 0.02}};

  /**
   * @brief Member variable for physics.
   */

  MFNode _physics{};

  /**
   * @brief Member variable for texCoord.
   */

  SFNode _texCoord{nullptr};

  /**
   * @brief Member variable for texCoordKey.
   */

  MFFloat _texCoordKey{};
};

} // namespace x3d::nodes
