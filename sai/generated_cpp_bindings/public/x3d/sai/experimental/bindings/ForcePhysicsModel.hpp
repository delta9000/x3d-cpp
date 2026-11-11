#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct ForcePhysicsModel {
  static constexpr std::string_view x3d_name = "ForcePhysicsModel";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ForcePhysicsModel, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel,
                                    ::x3d::sai::experimental::vec3f>
      force{"force", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {force.name(), force.kind, force.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
