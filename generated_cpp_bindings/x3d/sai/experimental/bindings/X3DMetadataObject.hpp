#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DMetadataObject {
  static constexpr std::string_view x3d_name = "X3DMetadataObject";
  inline static constexpr field_key<X3DMetadataObject, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<X3DMetadataObject, std::string> reference{
      "reference", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
