#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct OrientationDamper {
  static constexpr std::string_view x3d_name = "OrientationDamper";
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::rotation>
      initialDestination{"initialDestination", access_type::initialize_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::rotation>
      initialValue{"initialValue", access_type::initialize_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::rotation>
      set_destination{"set_destination", access_type::input_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::rotation>
      set_value{"set_value", access_type::input_only};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::time_value>
      tau{"tau", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, float> tolerance{
      "tolerance", access_type::input_output};
  inline static constexpr field_key<OrientationDamper,
                                    ::x3d::sai::experimental::rotation>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<OrientationDamper, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<OrientationDamper, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
