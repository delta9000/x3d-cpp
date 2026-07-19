#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsPositionInterpolator {
  static constexpr std::string_view x3d_name = "NurbsPositionInterpolator";
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::node_id>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::double_list>
      knot{"knot", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::int32_t>
      order{"order", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, float>
      set_fraction{"set_fraction", access_type::input_only};
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::vec3f>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<NurbsPositionInterpolator,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsPositionInterpolator, std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
