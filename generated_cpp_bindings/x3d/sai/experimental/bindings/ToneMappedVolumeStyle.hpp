#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ToneMappedVolumeStyle {
  static constexpr std::string_view x3d_name = "ToneMappedVolumeStyle";
  inline static constexpr field_key<ToneMappedVolumeStyle,
                                    ::x3d::sai::experimental::color4f>
      coolColor{"coolColor", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      surfaceNormals{"surfaceNormals", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle,
                                    ::x3d::sai::experimental::color4f>
      warmColor{"warmColor", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ToneMappedVolumeStyle, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
