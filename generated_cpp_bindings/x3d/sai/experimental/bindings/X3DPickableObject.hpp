#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DPickableObject {
  static constexpr std::string_view x3d_name = "X3DPickableObject";
  inline static constexpr field_key<X3DPickableObject, bool> pickable{
      "pickable", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
