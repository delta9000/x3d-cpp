#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct OrthoViewpoint {
  static constexpr std::string_view x3d_name = "OrthoViewpoint";
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::vec3f>
      centerOfRotation{"centerOfRotation", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::float_list>
      fieldOfView{"fieldOfView", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<OrthoViewpoint, bool> jump{
      "jump", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      navigationInfo{"navigationInfo", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, float> nearDistance{
      "nearDistance", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint,
                                    ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, bool> retainUserOffsets{
      "retainUserOffsets", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<OrthoViewpoint, bool> viewAll{
      "viewAll", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<OrthoViewpoint, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
