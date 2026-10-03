#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct FillProperties {
  static constexpr std::string_view x3d_name = "FillProperties";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<FillProperties, bool> filled{
      "filled", access_type::input_output};
  inline static constexpr field_key<FillProperties,
                                    ::x3d::sai::experimental::color3f>
      hatchColor{"hatchColor", access_type::input_output};
  inline static constexpr field_key<FillProperties, bool> hatched{
      "hatched", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::int32_t> hatchStyle{
      "hatchStyle", access_type::input_output};
  inline static constexpr field_key<FillProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {filled.name(), filled.kind, filled.access()},
      {hatchColor.name(), hatchColor.kind, hatchColor.access()},
      {hatched.name(), hatched.kind, hatched.access()},
      {hatchStyle.name(), hatchStyle.kind, hatchStyle.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
