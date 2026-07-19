#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Material {
  static constexpr std::string_view x3d_name = "Material";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Material, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      ambientTexture{"ambientTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      ambientTextureMapping{"ambientTextureMapping", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::color3f>
      diffuseColor{"diffuseColor", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      diffuseTexture{"diffuseTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      diffuseTextureMapping{"diffuseTextureMapping", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::color3f>
      emissiveColor{"emissiveColor", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      emissiveTexture{"emissiveTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      emissiveTextureMapping{"emissiveTextureMapping",
                             access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Material, float> normalScale{
      "normalScale", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      normalTexture{"normalTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string> normalTextureMapping{
      "normalTextureMapping", access_type::input_output};
  inline static constexpr field_key<Material, float> occlusionStrength{
      "occlusionStrength", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      occlusionTexture{"occlusionTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      occlusionTextureMapping{"occlusionTextureMapping",
                              access_type::input_output};
  inline static constexpr field_key<Material, float> shininess{
      "shininess", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      shininessTexture{"shininessTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      shininessTextureMapping{"shininessTextureMapping",
                              access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::color3f>
      specularColor{"specularColor", access_type::input_output};
  inline static constexpr field_key<Material, ::x3d::sai::experimental::node_id>
      specularTexture{"specularTexture", access_type::input_output};
  inline static constexpr field_key<Material, std::string>
      specularTextureMapping{"specularTextureMapping",
                             access_type::input_output};
  inline static constexpr field_key<Material, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<Material, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Material, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Material, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Material, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Material, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 29> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {ambientTexture.name(), ambientTexture.kind, ambientTexture.access()},
      {ambientTextureMapping.name(), ambientTextureMapping.kind,
       ambientTextureMapping.access()},
      {diffuseColor.name(), diffuseColor.kind, diffuseColor.access()},
      {diffuseTexture.name(), diffuseTexture.kind, diffuseTexture.access()},
      {diffuseTextureMapping.name(), diffuseTextureMapping.kind,
       diffuseTextureMapping.access()},
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
      {occlusionStrength.name(), occlusionStrength.kind,
       occlusionStrength.access()},
      {occlusionTexture.name(), occlusionTexture.kind,
       occlusionTexture.access()},
      {occlusionTextureMapping.name(), occlusionTextureMapping.kind,
       occlusionTextureMapping.access()},
      {shininess.name(), shininess.kind, shininess.access()},
      {shininessTexture.name(), shininessTexture.kind,
       shininessTexture.access()},
      {shininessTextureMapping.name(), shininessTextureMapping.kind,
       shininessTextureMapping.access()},
      {specularColor.name(), specularColor.kind, specularColor.access()},
      {specularTexture.name(), specularTexture.kind, specularTexture.access()},
      {specularTextureMapping.name(), specularTextureMapping.kind,
       specularTextureMapping.access()},
      {transparency.name(), transparency.kind, transparency.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
