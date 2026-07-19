#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ViewpointGroup {
  static constexpr std::string_view x3d_name = "ViewpointGroup";
  inline static constexpr field_key<ViewpointGroup,
                                    ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, bool> displayed{
      "displayed", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, bool> retainUserOffsets{
      "retainUserOffsets", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup,
                                    ::x3d::sai::experimental::vec3f>
      size{"size", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ViewpointGroup, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
