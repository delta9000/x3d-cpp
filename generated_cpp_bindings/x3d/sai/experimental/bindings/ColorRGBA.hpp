#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ColorRGBA {
  static constexpr std::string_view x3d_name = "ColorRGBA";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ColorRGBA,
                                    ::x3d::sai::experimental::color4f_list>
      color{"color", access_type::input_output};
  inline static constexpr field_key<ColorRGBA,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ColorRGBA,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ColorRGBA, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ColorRGBA, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ColorRGBA, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ColorRGBA, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ColorRGBA, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {color.name(), color.kind, color.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
