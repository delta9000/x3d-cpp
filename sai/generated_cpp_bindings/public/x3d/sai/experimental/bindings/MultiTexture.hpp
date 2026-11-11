#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct MultiTexture {
  static constexpr std::string_view x3d_name = "MultiTexture";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<MultiTexture, float> alpha{
      "alpha", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<MultiTexture, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::string_list>
      function{"function", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::string_list>
      mode{"mode", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::string_list>
      source{"source", access_type::input_output};
  inline static constexpr field_key<MultiTexture,
                                    ::x3d::sai::experimental::node_list>
      texture{"texture", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {alpha.name(), alpha.kind, alpha.access()},
      {color.name(), color.kind, color.access()},
      {description.name(), description.kind, description.access()},
      {function.name(), function.kind, function.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {mode.name(), mode.kind, mode.access()},
      {source.name(), source.kind, source.access()},
      {texture.name(), texture.kind, texture.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
