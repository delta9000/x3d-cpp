#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct WindPhysicsModel {
  static constexpr std::string_view x3d_name = "WindPhysicsModel";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<WindPhysicsModel,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<WindPhysicsModel, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<WindPhysicsModel, float> gustiness{
      "gustiness", access_type::input_output};
  inline static constexpr field_key<WindPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<WindPhysicsModel, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<WindPhysicsModel, float> turbulence{
      "turbulence", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {direction.name(), direction.kind, direction.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gustiness.name(), gustiness.kind, gustiness.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {speed.name(), speed.kind, speed.access()},
      {turbulence.name(), turbulence.kind, turbulence.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
