#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct KeySensor {
  static constexpr std::string_view x3d_name = "KeySensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<KeySensor, std::int32_t> actionKeyPress{
      "actionKeyPress", access_type::output_only};
  inline static constexpr field_key<KeySensor, std::int32_t> actionKeyRelease{
      "actionKeyRelease", access_type::output_only};
  inline static constexpr field_key<KeySensor, bool> altKey{
      "altKey", access_type::output_only};
  inline static constexpr field_key<KeySensor, bool> controlKey{
      "controlKey", access_type::output_only};
  inline static constexpr field_key<KeySensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<KeySensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<KeySensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<KeySensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<KeySensor, std::string> keyPress{
      "keyPress", access_type::output_only};
  inline static constexpr field_key<KeySensor, std::string> keyRelease{
      "keyRelease", access_type::output_only};
  inline static constexpr field_key<KeySensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<KeySensor, bool> shiftKey{
      "shiftKey", access_type::output_only};
  inline static constexpr field_key<KeySensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<KeySensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<KeySensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<KeySensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<KeySensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 17> field_keys{{
      {actionKeyPress.name(), actionKeyPress.kind, actionKeyPress.access()},
      {actionKeyRelease.name(), actionKeyRelease.kind,
       actionKeyRelease.access()},
      {altKey.name(), altKey.kind, altKey.access()},
      {controlKey.name(), controlKey.kind, controlKey.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {keyPress.name(), keyPress.kind, keyPress.access()},
      {keyRelease.name(), keyRelease.kind, keyRelease.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {shiftKey.name(), shiftKey.kind, shiftKey.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
