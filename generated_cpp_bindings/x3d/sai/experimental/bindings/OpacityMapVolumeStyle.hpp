#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct OpacityMapVolumeStyle {
  static constexpr std::string_view x3d_name = "OpacityMapVolumeStyle";
  inline static constexpr field_key<OpacityMapVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      transferFunction{"transferFunction", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
