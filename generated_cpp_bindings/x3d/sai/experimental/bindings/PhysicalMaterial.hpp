#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PhysicalMaterial {
  static constexpr std::string_view x3d_name = "PhysicalMaterial";
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::color3f>
      baseColor{"baseColor", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      baseTexture{"baseTexture", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string>
      baseTextureMapping{"baseTextureMapping", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::color3f>
      emissiveColor{"emissiveColor", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      emissiveTexture{"emissiveTexture", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string>
      emissiveTextureMapping{"emissiveTextureMapping",
                             access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, float> metallic{
      "metallic", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      metallicRoughnessTexture{"metallicRoughnessTexture",
                               access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string>
      metallicRoughnessTextureMapping{"metallicRoughnessTextureMapping",
                                      access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, float> normalScale{
      "normalScale", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      normalTexture{"normalTexture", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string>
      normalTextureMapping{"normalTextureMapping", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, float> occlusionStrength{
      "occlusionStrength", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial,
                                    ::x3d::sai::experimental::node_id>
      occlusionTexture{"occlusionTexture", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string>
      occlusionTextureMapping{"occlusionTextureMapping",
                              access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, float> roughness{
      "roughness", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PhysicalMaterial, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
