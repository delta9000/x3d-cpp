#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ColorInterpolator {
  static constexpr std::string_view x3d_name = "ColorInterpolator";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ColorInterpolator,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator,
                                    ::x3d::sai::experimental::color3f_list>
      keyValue{"keyValue", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator, float> set_fraction{
      "set_fraction", access_type::input_only};
  inline static constexpr field_key<ColorInterpolator,
                                    ::x3d::sai::experimental::color3f>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<ColorInterpolator, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ColorInterpolator, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {IS.name(), IS.kind, IS.access()},
      {key.name(), key.kind, key.access()},
      {keyValue.name(), keyValue.kind, keyValue.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_fraction.name(), set_fraction.kind, set_fraction.access()},
      {value_changed.name(), value_changed.kind, value_changed.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
