#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ClipPlane {
  static constexpr std::string_view x3d_name = "ClipPlane";
  inline static constexpr field_key<ClipPlane, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ClipPlane,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ClipPlane,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ClipPlane, ::x3d::sai::experimental::vec4f>
      plane{"plane", access_type::input_output};
  inline static constexpr field_key<ClipPlane, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ClipPlane, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ClipPlane, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ClipPlane, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ClipPlane, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
