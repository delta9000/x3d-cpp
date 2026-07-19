#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Sphere {
  static constexpr std::string_view x3d_name = "Sphere";
  inline static constexpr field_key<Sphere, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Sphere, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Sphere, float> radius{
      "radius", access_type::initialize_only};
  inline static constexpr field_key<Sphere, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Sphere, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Sphere, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Sphere, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Sphere, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Sphere, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
