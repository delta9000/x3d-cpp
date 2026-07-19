#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

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
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ForcePhysicsModel, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {force.name(), force.kind, force.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
