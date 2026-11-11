#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct LOD {
  static constexpr std::string_view x3d_name = "LOD";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<LOD, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      center{"center", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LOD, bool> forceTransitions{
      "forceTransitions", access_type::initialize_only};
  inline static constexpr field_key<LOD, std::int32_t> level_changed{
      "level_changed", access_type::output_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::float_list>
      range{"range", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<LOD, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {center.name(), center.kind, center.access()},
      {children.name(), children.kind, children.access()},
      {forceTransitions.name(), forceTransitions.kind,
       forceTransitions.access()},
      {level_changed.name(), level_changed.kind, level_changed.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {range.name(), range.kind, range.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
