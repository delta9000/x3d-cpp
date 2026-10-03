#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PixelTexture3D {
  static constexpr std::string_view x3d_name = "PixelTexture3D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PixelTexture3D, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PixelTexture3D,
                                    ::x3d::sai::experimental::int32_list>
      image{"image", access_type::input_output};
  inline static constexpr field_key<PixelTexture3D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PixelTexture3D, bool> repeatR{
      "repeatR", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture3D, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture3D, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture3D,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 7> field_keys{{
      {description.name(), description.kind, description.access()},
      {image.name(), image.kind, image.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {repeatR.name(), repeatR.kind, repeatR.access()},
      {repeatS.name(), repeatS.kind, repeatS.access()},
      {repeatT.name(), repeatT.kind, repeatT.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
