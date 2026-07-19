#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NavigationInfo {
  static constexpr std::string_view x3d_name = "NavigationInfo";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::float_list>
      avatarSize{"avatarSize", access_type::input_output};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<NavigationInfo, bool> headlight{
      "headlight", access_type::input_output};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<NavigationInfo, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, bool> transitionComplete{
      "transitionComplete", access_type::output_only};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::time_value>
      transitionTime{"transitionTime", access_type::input_output};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::string_list>
      transitionType{"transitionType", access_type::input_output};
  inline static constexpr field_key<NavigationInfo,
                                    ::x3d::sai::experimental::string_list>
      type{"type", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, float> visibilityLimit{
      "visibilityLimit", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NavigationInfo, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 18> field_keys{{
      {avatarSize.name(), avatarSize.kind, avatarSize.access()},
      {bindTime.name(), bindTime.kind, bindTime.access()},
      {headlight.name(), headlight.kind, headlight.access()},
      {IS.name(), IS.kind, IS.access()},
      {isBound.name(), isBound.kind, isBound.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_bind.name(), set_bind.kind, set_bind.access()},
      {speed.name(), speed.kind, speed.access()},
      {transitionComplete.name(), transitionComplete.kind,
       transitionComplete.access()},
      {transitionTime.name(), transitionTime.kind, transitionTime.access()},
      {transitionType.name(), transitionType.kind, transitionType.access()},
      {type.name(), type.kind, type.access()},
      {visibilityLimit.name(), visibilityLimit.kind, visibilityLimit.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
