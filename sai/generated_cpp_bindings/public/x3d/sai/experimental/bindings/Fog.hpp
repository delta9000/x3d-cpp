#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct Fog {
  static constexpr std::string_view x3d_name = "Fog";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> fogType{
      "fogType", access_type::input_output};
  inline static constexpr field_key<Fog, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Fog, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<Fog, float> visibilityRange{
      "visibilityRange", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 7> field_keys{{
      {bindTime.name(), bindTime.kind, bindTime.access()},
      {color.name(), color.kind, color.access()},
      {fogType.name(), fogType.kind, fogType.access()},
      {isBound.name(), isBound.kind, isBound.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_bind.name(), set_bind.kind, set_bind.access()},
      {visibilityRange.name(), visibilityRange.kind, visibilityRange.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
