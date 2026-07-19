#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ColorChaser {
  static constexpr std::string_view x3d_name = "ColorChaser";
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::time_value>
      duration{"duration", access_type::initialize_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::color3f>
      initialDestination{"initialDestination", access_type::initialize_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::color3f>
      initialValue{"initialValue", access_type::initialize_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ColorChaser, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::color3f>
      set_destination{"set_destination", access_type::input_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::color3f>
      set_value{"set_value", access_type::input_only};
  inline static constexpr field_key<ColorChaser,
                                    ::x3d::sai::experimental::color3f>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<ColorChaser, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ColorChaser, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ColorChaser, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ColorChaser, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ColorChaser, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
