#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DPickableObject {
  static constexpr std::string_view x3d_name = "X3DPickableObject";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DPickableObject, bool> pickable{
      "pickable", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 1> field_keys{{
      {pickable.name(), pickable.kind, pickable.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
