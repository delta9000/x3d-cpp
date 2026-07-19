#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DFogObject {
  static constexpr std::string_view x3d_name = "X3DFogObject";
  inline static constexpr field_key<X3DFogObject,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<X3DFogObject,
                                    ::x3d::sai::experimental::enum_value>
      fogType{"fogType", access_type::input_output};
  inline static constexpr field_key<X3DFogObject, float> visibilityRange{
      "visibilityRange", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
