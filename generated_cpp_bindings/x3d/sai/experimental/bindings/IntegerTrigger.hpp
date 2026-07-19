#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IntegerTrigger {
  static constexpr std::string_view x3d_name = "IntegerTrigger";
  inline static constexpr field_key<IntegerTrigger, std::int32_t> integerKey{
      "integerKey", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, bool> set_boolean{
      "set_boolean", access_type::input_only};
  inline static constexpr field_key<IntegerTrigger, std::int32_t> triggerValue{
      "triggerValue", access_type::output_only};
  inline static constexpr field_key<IntegerTrigger, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
