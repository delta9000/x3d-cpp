#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DShapeNode {
  static constexpr std::string_view x3d_name = "X3DShapeNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      appearance{"appearance", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<X3DShapeNode, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<X3DShapeNode, bool> castShadow{
      "castShadow", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {appearance.name(), appearance.kind, appearance.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {castShadow.name(), castShadow.kind, castShadow.access()},
      {geometry.name(), geometry.kind, geometry.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
