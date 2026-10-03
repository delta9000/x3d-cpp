#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DNBodyCollisionSpaceNode {
  static constexpr std::string_view x3d_name = "X3DNBodyCollisionSpaceNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode, bool>
      bboxDisplay{"bboxDisplay", access_type::input_output};
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DNBodyCollisionSpaceNode, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
