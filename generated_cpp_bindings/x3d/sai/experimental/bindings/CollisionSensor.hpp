#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CollisionSensor {
  static constexpr std::string_view x3d_name = "CollisionSensor";
  inline static constexpr field_key<CollisionSensor,
                                    ::x3d::sai::experimental::node_id>
      collider{"collider", access_type::input_output};
  inline static constexpr field_key<CollisionSensor,
                                    ::x3d::sai::experimental::node_list>
      contacts{"contacts", access_type::output_only};
  inline static constexpr field_key<CollisionSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CollisionSensor,
                                    ::x3d::sai::experimental::node_list>
      intersections{"intersections", access_type::output_only};
  inline static constexpr field_key<CollisionSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<CollisionSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CollisionSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
