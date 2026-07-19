#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PixelTexture {
  static constexpr std::string_view x3d_name = "PixelTexture";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PixelTexture, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::image>
      image{"image", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PixelTexture, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {description.name(), description.kind, description.access()},
      {image.name(), image.kind, image.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {repeatS.name(), repeatS.kind, repeatS.access()},
      {repeatT.name(), repeatT.kind, repeatT.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
