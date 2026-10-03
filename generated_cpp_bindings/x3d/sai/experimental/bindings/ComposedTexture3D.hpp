#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ComposedTexture3D {
  static constexpr std::string_view x3d_name = "ComposedTexture3D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ComposedTexture3D, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatR{
      "repeatR", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_list>
      texture{"texture", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 7> field_keys{{
      {description.name(), description.kind, description.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {repeatR.name(), repeatR.kind, repeatR.access()},
      {repeatS.name(), repeatS.kind, repeatS.access()},
      {repeatT.name(), repeatT.kind, repeatT.access()},
      {texture.name(), texture.kind, texture.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
