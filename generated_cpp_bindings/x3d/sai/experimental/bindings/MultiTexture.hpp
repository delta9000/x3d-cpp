#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

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
      IS{"IS", access_type::input_output};
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
  inline static constexpr field_key<MultiTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<MultiTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<MultiTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<MultiTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<MultiTexture, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {alpha.name(), alpha.kind, alpha.access()},
      {color.name(), color.kind, color.access()},
      {description.name(), description.kind, description.access()},
      {function.name(), function.kind, function.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {mode.name(), mode.kind, mode.access()},
      {source.name(), source.kind, source.access()},
      {texture.name(), texture.kind, texture.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
