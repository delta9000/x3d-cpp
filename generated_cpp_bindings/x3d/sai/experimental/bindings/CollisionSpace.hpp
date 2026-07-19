#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CollisionSpace {
  static constexpr std::string_view x3d_name = "CollisionSpace";
  inline static constexpr field_key<CollisionSpace,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<CollisionSpace, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<CollisionSpace,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<CollisionSpace,
                                    ::x3d::sai::experimental::node_list>
      collidables{"collidables", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CollisionSpace,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CollisionSpace,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, bool> useGeometry{
      "useGeometry", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CollisionSpace, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
