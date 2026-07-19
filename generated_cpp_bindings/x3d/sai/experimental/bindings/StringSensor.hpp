#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct StringSensor {
  static constexpr std::string_view x3d_name = "StringSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<StringSensor, bool> deletionAllowed{
      "deletionAllowed", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<StringSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> enteredText{
      "enteredText", access_type::output_only};
  inline static constexpr field_key<StringSensor, std::string> finalText{
      "finalText", access_type::output_only};
  inline static constexpr field_key<StringSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<StringSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<StringSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<StringSensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {deletionAllowed.name(), deletionAllowed.kind, deletionAllowed.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {enteredText.name(), enteredText.kind, enteredText.access()},
      {finalText.name(), finalText.kind, finalText.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
