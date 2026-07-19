#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DMetadataObject {
  static constexpr std::string_view x3d_name = "X3DMetadataObject";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DMetadataObject, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<X3DMetadataObject, std::string> reference{
      "reference", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {name.name(), name.kind, name.access()},
      {reference.name(), reference.kind, reference.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
