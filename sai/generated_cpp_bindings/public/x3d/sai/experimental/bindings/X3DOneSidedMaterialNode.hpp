#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DOneSidedMaterialNode {
  static constexpr std::string_view x3d_name = "X3DOneSidedMaterialNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DOneSidedMaterialNode, std::string>
      emissiveTextureMapping{"emissiveTextureMapping",
                             access_type::input_output};
  inline static constexpr field_key<X3DOneSidedMaterialNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DOneSidedMaterialNode, float> normalScale{
      "normalScale", access_type::input_output};
  inline static constexpr field_key<X3DOneSidedMaterialNode, std::string>
      normalTextureMapping{"normalTextureMapping", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {emissiveTextureMapping.name(), emissiveTextureMapping.kind,
       emissiveTextureMapping.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normalScale.name(), normalScale.kind, normalScale.access()},
      {normalTextureMapping.name(), normalTextureMapping.kind,
       normalTextureMapping.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
