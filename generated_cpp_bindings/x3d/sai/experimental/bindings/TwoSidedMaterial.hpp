#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TwoSidedMaterial {
  static constexpr std::string_view x3d_name = "TwoSidedMaterial";
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
};

} // namespace x3d::sai::experimental::bindings
