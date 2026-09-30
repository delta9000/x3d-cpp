#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Cone {
  static constexpr std::string_view x3d_name = "Cone";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Cone, bool> bottom{
      "bottom", access_type::input_output};
  inline static constexpr field_key<Cone, float> bottomRadius{
      "bottomRadius", access_type::initialize_only};
  inline static constexpr field_key<Cone, float> height{
      "height", access_type::initialize_only};
  inline static constexpr field_key<Cone, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Cone, bool> side{"side",
                                                     access_type::input_output};
  inline static constexpr field_key<Cone, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {bottom.name(), bottom.kind, bottom.access()},
      {bottomRadius.name(), bottomRadius.kind, bottomRadius.access()},
      {height.name(), height.kind, height.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {side.name(), side.kind, side.access()},
      {solid.name(), solid.kind, solid.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
