#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PositionInterpolator {
  static constexpr std::string_view x3d_name = "PositionInterpolator";
  inline static constexpr field_key<PositionInterpolator,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator,
                                    ::x3d::sai::experimental::vec3f_list>
      keyValue{"keyValue", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator, float> set_fraction{
      "set_fraction", access_type::input_only};
  inline static constexpr field_key<PositionInterpolator,
                                    ::x3d::sai::experimental::vec3f>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<PositionInterpolator, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PositionInterpolator, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
