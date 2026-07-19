#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ScalarChaser {
  static constexpr std::string_view x3d_name = "ScalarChaser";
  inline static constexpr field_key<ScalarChaser,
                                    ::x3d::sai::experimental::time_value>
      duration{"duration", access_type::initialize_only};
  inline static constexpr field_key<ScalarChaser, float> initialDestination{
      "initialDestination", access_type::initialize_only};
  inline static constexpr field_key<ScalarChaser, float> initialValue{
      "initialValue", access_type::initialize_only};
  inline static constexpr field_key<ScalarChaser,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<ScalarChaser,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, float> set_destination{
      "set_destination", access_type::input_only};
  inline static constexpr field_key<ScalarChaser, float> set_value{
      "set_value", access_type::input_only};
  inline static constexpr field_key<ScalarChaser, float> value_changed{
      "value_changed", access_type::output_only};
  inline static constexpr field_key<ScalarChaser, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ScalarChaser, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
