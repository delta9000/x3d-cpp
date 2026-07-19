#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Text {
  static constexpr std::string_view x3d_name = "Text";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Text, ::x3d::sai::experimental::node_id>
      fontStyle{"fontStyle", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::node_id> IS{
      "IS", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::float_list>
      length{"length", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::vec2f_list>
      lineBounds{"lineBounds", access_type::output_only};
  inline static constexpr field_key<Text, float> maxExtent{
      "maxExtent", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::vec3f>
      origin{"origin", access_type::output_only};
  inline static constexpr field_key<Text, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::string_list>
      string{"string", access_type::input_output};
  inline static constexpr field_key<Text, ::x3d::sai::experimental::vec2f>
      textBounds{"textBounds", access_type::output_only};
  inline static constexpr field_key<Text, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Text, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Text, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Text, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Text, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {fontStyle.name(), fontStyle.kind, fontStyle.access()},
      {IS.name(), IS.kind, IS.access()},
      {length.name(), length.kind, length.access()},
      {lineBounds.name(), lineBounds.kind, lineBounds.access()},
      {maxExtent.name(), maxExtent.kind, maxExtent.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {origin.name(), origin.kind, origin.access()},
      {solid.name(), solid.kind, solid.access()},
      {string.name(), string.kind, string.access()},
      {textBounds.name(), textBounds.kind, textBounds.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
