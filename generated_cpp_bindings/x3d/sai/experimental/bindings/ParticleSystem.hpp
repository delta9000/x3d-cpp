#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ParticleSystem {
  static constexpr std::string_view x3d_name = "ParticleSystem";
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
};

} // namespace x3d::sai::experimental::bindings
