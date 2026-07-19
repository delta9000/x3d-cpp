#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Material {
  static constexpr std::string_view x3d_name = "Material";
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
};

} // namespace x3d::sai::experimental::bindings
