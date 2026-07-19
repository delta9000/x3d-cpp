#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Viewpoint {
  static constexpr std::string_view x3d_name = "Viewpoint";
  inline static constexpr field_key<Viewpoint,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<Viewpoint, ::x3d::sai::experimental::vec3f>
      centerOfRotation{"centerOfRotation", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Viewpoint, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<Viewpoint, float> fieldOfView{
      "fieldOfView", access_type::input_output};
  inline static constexpr field_key<Viewpoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Viewpoint, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<Viewpoint, bool> jump{
      "jump", access_type::input_output};
  inline static constexpr field_key<Viewpoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Viewpoint,
                                    ::x3d::sai::experimental::node_id>
      navigationInfo{"navigationInfo", access_type::input_output};
  inline static constexpr field_key<Viewpoint, float> nearDistance{
      "nearDistance", access_type::input_output};
  inline static constexpr field_key<Viewpoint,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<Viewpoint, ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<Viewpoint, bool> retainUserOffsets{
      "retainUserOffsets", access_type::input_output};
  inline static constexpr field_key<Viewpoint, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<Viewpoint, bool> viewAll{
      "viewAll", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Viewpoint, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
