#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DBoundedObject {
  static constexpr std::string_view x3d_name = "X3DBoundedObject";
  inline static constexpr field_key<X3DBoundedObject,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<X3DBoundedObject, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<X3DBoundedObject,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<X3DBoundedObject, bool> visible{
      "visible", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
