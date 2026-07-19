#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ParticleSystem {
  static constexpr std::string_view x3d_name = "ParticleSystem";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      appearance{"appearance", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem, bool> castShadow{
      "castShadow", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::float_list>
      colorKey{"colorKey", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem, bool> createParticles{
      "createParticles", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      emitter{"emitter", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> geometryType{
      "geometryType", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<ParticleSystem, float> lifetimeVariation{
      "lifetimeVariation", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::int32_t> maxParticles{
      "maxParticles", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, float> particleLifetime{
      "particleLifetime", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::vec2f>
      particleSize{"particleSize", access_type::input_output};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_list>
      physics{"physics", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem,
                                    ::x3d::sai::experimental::float_list>
      texCoordKey{"texCoordKey", access_type::initialize_only};
  inline static constexpr field_key<ParticleSystem, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ParticleSystem, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 28> field_keys{{
      {appearance.name(), appearance.kind, appearance.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {castShadow.name(), castShadow.kind, castShadow.access()},
      {color.name(), color.kind, color.access()},
      {colorKey.name(), colorKey.kind, colorKey.access()},
      {createParticles.name(), createParticles.kind, createParticles.access()},
      {emitter.name(), emitter.kind, emitter.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {geometry.name(), geometry.kind, geometry.access()},
      {geometryType.name(), geometryType.kind, geometryType.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {lifetimeVariation.name(), lifetimeVariation.kind,
       lifetimeVariation.access()},
      {maxParticles.name(), maxParticles.kind, maxParticles.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {particleLifetime.name(), particleLifetime.kind,
       particleLifetime.access()},
      {particleSize.name(), particleSize.kind, particleSize.access()},
      {physics.name(), physics.kind, physics.access()},
      {texCoord.name(), texCoord.kind, texCoord.access()},
      {texCoordKey.name(), texCoordKey.kind, texCoordKey.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
