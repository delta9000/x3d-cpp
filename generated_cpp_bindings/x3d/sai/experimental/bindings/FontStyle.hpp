#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct FontStyle {
  static constexpr std::string_view x3d_name = "FontStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<FontStyle, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<FontStyle,
                                    ::x3d::sai::experimental::string_list>
      family{"family", access_type::input_output};
  inline static constexpr field_key<FontStyle, bool> horizontal{
      "horizontal", access_type::input_output};
  inline static constexpr field_key<FontStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<FontStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<FontStyle,
                                    ::x3d::sai::experimental::enum_list>
      justify{"justify", access_type::input_output};
  inline static constexpr field_key<FontStyle, std::string> language{
      "language", access_type::input_output};
  inline static constexpr field_key<FontStyle, bool> leftToRight{
      "leftToRight", access_type::input_output};
  inline static constexpr field_key<FontStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<FontStyle, float> size{
      "size", access_type::input_output};
  inline static constexpr field_key<FontStyle, float> spacing{
      "spacing", access_type::input_output};
  inline static constexpr field_key<FontStyle,
                                    ::x3d::sai::experimental::enum_value>
      style{"style", access_type::input_output};
  inline static constexpr field_key<FontStyle, bool> topToBottom{
      "topToBottom", access_type::input_output};
  inline static constexpr field_key<FontStyle, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<FontStyle, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {class_.name(), class_.kind, class_.access()},
      {family.name(), family.kind, family.access()},
      {horizontal.name(), horizontal.kind, horizontal.access()},
      {id.name(), id.kind, id.access()},
      {IS.name(), IS.kind, IS.access()},
      {justify.name(), justify.kind, justify.access()},
      {language.name(), language.kind, language.access()},
      {leftToRight.name(), leftToRight.kind, leftToRight.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {size.name(), size.kind, size.access()},
      {spacing.name(), spacing.kind, spacing.access()},
      {style.name(), style.kind, style.access()},
      {topToBottom.name(), topToBottom.kind, topToBottom.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
