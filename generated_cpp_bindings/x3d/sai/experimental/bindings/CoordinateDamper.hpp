#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CoordinateDamper {
  static constexpr std::string_view x3d_name = "CoordinateDamper";
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      initialDestination{"initialDestination", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      initialValue{"initialValue", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      set_destination{"set_destination", access_type::input_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      set_value{"set_value", access_type::input_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::time_value>
      tau{"tau", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, float> tolerance{
      "tolerance", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<CoordinateDamper, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
