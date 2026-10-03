#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LocalFog {
  static constexpr std::string_view x3d_name = "LocalFog";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LocalFog, ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<LocalFog, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> fogType{
      "fogType", access_type::input_output};
  inline static constexpr field_key<LocalFog, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LocalFog, float> visibilityRange{
      "visibilityRange", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {color.name(), color.kind, color.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {fogType.name(), fogType.kind, fogType.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {visibilityRange.name(), visibilityRange.kind, visibilityRange.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
