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
  ParticleSystem() = default;

  ~ParticleSystem() = default;

  static SFNode getDefaultColor() { return nullptr; }

  static SFBool getDefaultCreateParticles() { return true; }

  static SFNode getDefaultEmitter() { return nullptr; }

  static SFBool getDefaultEnabled() { return true; }

  static SFNode getDefaultGeometry() { return nullptr; }

  static SFString getDefaultGeometryType() { return "QUAD"; }

  static SFFloat getDefaultLifetimeVariation() { return 0.25; }

  static SFInt32 getDefaultMaxParticles() { return 200; }

  static SFFloat getDefaultParticleLifetime() { return 5; }

  static SFVec2f getDefaultParticleSize() { return SFVec2f{0.02, 0.02}; }

  static SFNode getDefaultTexCoord() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The color field contains Color|ColorRGBA nodes as a series of
   * color values to be used at the given colorKey points in time.
   */
  const SFNode &getColor() const { return _color; }

  /**
   * @details Permitted X3D node types: X3DColorNode
   */
  static const std::vector<std::string> &acceptableColorNodeTypes() {
    static const std::vector<std::string> types = {"X3DColorNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColorUnchecked(const SFNode &value) { _color = value; }

  /**
   * MFFloat; access: initializeOnly. Borrows the stored value.
   * @details Array of time intervals in seconds, corresponding to particle
   * lifetime, that are used to interpolate color array values.
   */
  const MFFloat &getColorKey() const { return _colorKey; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColorKeyUnchecked(const MFFloat &value) { _colorKey = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables creation of new particles, while any existing
   * particles remain in existence and continue to animate until the end of
   * their lifetimes.
   */
  const SFBool &getCreateParticles() const { return _createParticles; }

  /**
   * Access: inputOutput.
   * @details Enables/disables creation of new particles, while any existing
   * particles remain in existence and continue to animate until the end of
   * their lifetimes.
   */
  void setCreateParticles(const SFBool &value) { _createParticles = value; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The emitter field specifies the type of emitter geometry and
   * properties that the particles are given for their initial positions.
   */
  const SFNode &getEmitter() const { return _emitter; }

  /**
   * @details Permitted X3D node types: X3DParticleEmitterNode
   */
  static const std::vector<std::string> &acceptableEmitterNodeTypes() {
    static const std::vector<std::string> types = {"X3DParticleEmitterNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setEmitterUnchecked(const SFNode &value) { _emitter = value; }

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
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained geometry node provides geometry used for each
   * particle when geometryType=GEOMETRY.
   */
  const SFNode &getGeometry() const { return _geometry; }

  /**
   * @details Permitted X3D node types: X3DGeometryNode
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained geometry node provides geometry used for each
   * particle when geometryType=GEOMETRY.
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details specifies type of geometry used to represent individual particles.
   */
  const SFString &getGeometryType() const { return _geometryType; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeometryTypeUnchecked(const SFString &value) {
    _geometryType = value;
  }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details isActive true/false events are sent when playback starts/stops.
   */
  const SFBool &getIsActive() const { return _isActive; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details isActive true/false events are sent when playback starts/stops.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details TODO not properly defined in X3D spedification.
   */
  const SFFloat &getLifetimeVariation() const { return _lifetimeVariation; }

  /**
   * Access: inputOutput.
   * @details TODO not properly defined in X3D spedification.
   */
  void setLifetimeVariation(const SFFloat &value) {

    validateLifetimeVariation(value);

    _lifetimeVariation = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLifetimeVariation() for range validation.
   */
  void setLifetimeVariationUnchecked(const SFFloat &value) {
    _lifetimeVariation = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Maximum number of particles to be generated at one time (subject
   * to player limitations).
   */
  const SFInt32 &getMaxParticles() const { return _maxParticles; }

  /**
   * Access: inputOutput.
   * @details Maximum number of particles to be generated at one time (subject
   * to player limitations).
   */
  void setMaxParticles(const SFInt32 &value) {

    validateMaxParticles(value);

    _maxParticles = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMaxParticles() for range validation.
   */
  void setMaxParticlesUnchecked(const SFInt32 &value) { _maxParticles = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details TODO not properly defined in X3D spedification.
   */
  const SFFloat &getParticleLifetime() const { return _particleLifetime; }

  /**
   * Access: inputOutput.
   * @details TODO not properly defined in X3D spedification.
   */
  void setParticleLifetime(const SFFloat &value) {

    validateParticleLifetime(value);

    _particleLifetime = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setParticleLifetime() for range validation.
   */
  void setParticleLifetimeUnchecked(const SFFloat &value) {
    _particleLifetime = value;
  }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details particleSize describes width and height dimensions for each
   * particle in length base units (default is meters).
   */
  const SFVec2f &getParticleSize() const { return _particleSize; }

  /**
   * Access: inputOutput.
   * @details particleSize describes width and height dimensions for each
   * particle in length base units (default is meters).
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setParticleSize() for range validation.
   */
  void setParticleSizeUnchecked(const SFVec2f &value) { _particleSize = value; }

  /**
   * MFNode; access: initializeOnly. Borrows the stored value.
   * @details After being created, the individual particles are then manipulated
   * according to the physics model(s) specified in the physics field.
   */
  const MFNode &getPhysics() const { return _physics; }

  /**
   * @details Permitted X3D node types: X3DParticlePhysicsModelNode
   */
  static const std::vector<std::string> &acceptablePhysicsNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DParticlePhysicsModelNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setPhysicsUnchecked(const MFNode &value) { _physics = value; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details texture coordinates of the provided texture(s) in the Appearance
   * node, over time.
   */
  const SFNode &getTexCoord() const { return _texCoord; }

  /**
   * @details Permitted X3D node types: TextureCoordinate,
   * TextureCoordinateGenerator
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "TextureCoordinate", "TextureCoordinateGenerator"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexCoordUnchecked(const SFNode &value) { _texCoord = value; }

  /**
   * MFFloat; access: initializeOnly. Borrows the stored value.
   * @details Array of time intervals in seconds, corresponding to particle
   * lifetime, that are used to sequence texCoord array values.
   */
  const MFFloat &getTexCoordKey() const { return _texCoordKey; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexCoordKeyUnchecked(const MFFloat &value) { _texCoordKey = value; }

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

    if (value < 0.0f)
      throw std::out_of_range("lifetimeVariation below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("lifetimeVariation above maximum of 1");
  }

  static void validateMaxParticles(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("maxParticles below minimum of 0");
  }

  static void validateParticleLifetime(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("particleLifetime below minimum of 0");
  }

  static void validateParticleSize(const SFVec2f &value) {

    if (value.x < 0.0f)
      throw std::out_of_range("particleSize.x below minimum of 0");

    if (value.y < 0.0f)
      throw std::out_of_range("particleSize.y below minimum of 0");
  }

  SFNode _color{nullptr};

  MFFloat _colorKey{};

  SFBool _createParticles{true};

  SFNode _emitter{nullptr};

  SFBool _enabled{true};

  SFNode _geometry{nullptr};

  SFString _geometryType{"QUAD"};

  SFBool _isActive{};

  SFFloat _lifetimeVariation{0.25};

  SFInt32 _maxParticles{200};

  SFFloat _particleLifetime{5};

  SFVec2f _particleSize{SFVec2f{0.02, 0.02}};

  MFNode _physics{};

  SFNode _texCoord{nullptr};

  MFFloat _texCoordKey{};
};

} // namespace x3d::nodes
