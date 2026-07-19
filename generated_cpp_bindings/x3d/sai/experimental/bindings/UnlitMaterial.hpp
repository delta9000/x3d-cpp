#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct UnlitMaterial {
  static constexpr std::string_view x3d_name = "UnlitMaterial";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<UnlitMaterial,
                                    ::x3d::sai::experimental::color3f>
      emissiveColor{"emissiveColor", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial,
                                    ::x3d::sai::experimental::node_id>
      emissiveTexture{"emissiveTexture", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string>
      emissiveTextureMapping{"emissiveTextureMapping",
                             access_type::input_output};
  inline static constexpr field_key<UnlitMaterial,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, float> normalScale{
      "normalScale", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial,
                                    ::x3d::sai::experimental::node_id>
      normalTexture{"normalTexture", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string>
      normalTextureMapping{"normalTextureMapping", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<UnlitMaterial, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {emissiveColor.name(), emissiveColor.kind, emissiveColor.access()},
      {emissiveTexture.name(), emissiveTexture.kind, emissiveTexture.access()},
      {emissiveTextureMapping.name(), emissiveTextureMapping.kind,
       emissiveTextureMapping.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normalScale.name(), normalScale.kind, normalScale.access()},
      {normalTexture.name(), normalTexture.kind, normalTexture.access()},
      {normalTextureMapping.name(), normalTextureMapping.kind,
       normalTextureMapping.access()},
      {transparency.name(), transparency.kind, transparency.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
