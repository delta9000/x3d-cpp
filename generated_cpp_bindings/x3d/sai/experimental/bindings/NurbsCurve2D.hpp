#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsCurve2D {
  static constexpr std::string_view x3d_name = "NurbsCurve2D";
  inline static constexpr field_key<NurbsCurve2D, bool> closed{
      "closed", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::vec2d_list>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::double_list>
      knot{"knot", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D, std::int32_t> tessellation{
      "tessellation", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
