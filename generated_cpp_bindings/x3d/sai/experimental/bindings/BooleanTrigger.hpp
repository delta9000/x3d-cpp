#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BooleanTrigger {
  static constexpr std::string_view x3d_name = "BooleanTrigger";
  inline static constexpr field_key<BooleanTrigger,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger,
                                    ::x3d::sai::experimental::time_value>
      set_triggerTime{"set_triggerTime", access_type::input_only};
  inline static constexpr field_key<BooleanTrigger, bool> triggerTrue{
      "triggerTrue", access_type::output_only};
  inline static constexpr field_key<BooleanTrigger, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
