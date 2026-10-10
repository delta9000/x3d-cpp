#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct PickableGroup {
  static constexpr std::string_view x3d_name = "PickableGroup";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<PickableGroup, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<PickableGroup, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PickableGroup, bool> pickable{
      "pickable", access_type::input_output};
  inline static constexpr field_key<PickableGroup,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<PickableGroup, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pickable.name(), pickable.kind, pickable.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
