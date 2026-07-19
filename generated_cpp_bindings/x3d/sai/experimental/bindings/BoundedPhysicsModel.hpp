#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BoundedPhysicsModel {
  static constexpr std::string_view x3d_name = "BoundedPhysicsModel";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<BoundedPhysicsModel, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {geometry.name(), geometry.kind, geometry.access()},
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
