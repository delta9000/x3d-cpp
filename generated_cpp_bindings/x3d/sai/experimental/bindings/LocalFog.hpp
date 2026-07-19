#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LocalFog {
  static constexpr std::string_view x3d_name = "LocalFog";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LocalFog, ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<LocalFog, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<LocalFog,
                                    ::x3d::sai::experimental::enum_value>
      fogType{"fogType", access_type::input_output};
  inline static constexpr field_key<LocalFog, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LocalFog, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LocalFog, float> visibilityRange{
      "visibilityRange", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LocalFog, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {color.name(), color.kind, color.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {fogType.name(), fogType.kind, fogType.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {visibilityRange.name(), visibilityRange.kind, visibilityRange.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
