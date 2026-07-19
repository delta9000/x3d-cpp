#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ViewpointGroup {
  static constexpr std::string_view x3d_name = "ViewpointGroup";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {center.name(), center.kind, center.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {displayed.name(), displayed.kind, displayed.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {retainUserOffsets.name(), retainUserOffsets.kind,
       retainUserOffsets.access()},
      {size.name(), size.kind, size.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
