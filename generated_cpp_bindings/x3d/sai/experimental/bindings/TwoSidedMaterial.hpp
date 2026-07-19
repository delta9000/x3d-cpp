#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TwoSidedMaterial {
  static constexpr std::string_view x3d_name = "TwoSidedMaterial";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TwoSidedMaterial, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, float>
      backAmbientIntensity{"backAmbientIntensity", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      backDiffuseColor{"backDiffuseColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      backEmissiveColor{"backEmissiveColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, float> backShininess{
      "backShininess", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      backSpecularColor{"backSpecularColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, float> backTransparency{
      "backTransparency", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      diffuseColor{"diffuseColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      emissiveColor{"emissiveColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, bool> separateBackColor{
      "separateBackColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, float> shininess{
      "shininess", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial,
                                    ::x3d::sai::experimental::color3f>
      specularColor{"specularColor", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TwoSidedMaterial, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 20> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {backAmbientIntensity.name(), backAmbientIntensity.kind,
       backAmbientIntensity.access()},
      {backDiffuseColor.name(), backDiffuseColor.kind,
       backDiffuseColor.access()},
      {backEmissiveColor.name(), backEmissiveColor.kind,
       backEmissiveColor.access()},
      {backShininess.name(), backShininess.kind, backShininess.access()},
      {backSpecularColor.name(), backSpecularColor.kind,
       backSpecularColor.access()},
      {backTransparency.name(), backTransparency.kind,
       backTransparency.access()},
      {diffuseColor.name(), diffuseColor.kind, diffuseColor.access()},
      {emissiveColor.name(), emissiveColor.kind, emissiveColor.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {separateBackColor.name(), separateBackColor.kind,
       separateBackColor.access()},
      {shininess.name(), shininess.kind, shininess.access()},
      {specularColor.name(), specularColor.kind, specularColor.access()},
      {transparency.name(), transparency.kind, transparency.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
