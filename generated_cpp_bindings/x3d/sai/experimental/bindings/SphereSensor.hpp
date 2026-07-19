#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SphereSensor {
  static constexpr std::string_view x3d_name = "SphereSensor";
  inline static constexpr field_key<SphereSensor, bool> autoOffset{
      "autoOffset", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<SphereSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SphereSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<SphereSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::rotation>
      offset{"offset", access_type::input_output};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::rotation>
      rotation_changed{"rotation_changed", access_type::output_only};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::vec3f>
      trackPoint_changed{"trackPoint_changed", access_type::output_only};
  inline static constexpr field_key<SphereSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
