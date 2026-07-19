#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Arc2D {
  static constexpr std::string_view x3d_name = "Arc2D";
  inline static constexpr field_key<Arc2D, float> endAngle{
      "endAngle", access_type::initialize_only};
  inline static constexpr field_key<Arc2D, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Arc2D, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Arc2D, float> radius{
      "radius", access_type::initialize_only};
  inline static constexpr field_key<Arc2D, float> startAngle{
      "startAngle", access_type::initialize_only};
  inline static constexpr field_key<Arc2D, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Arc2D, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Arc2D, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Arc2D, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Arc2D, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
