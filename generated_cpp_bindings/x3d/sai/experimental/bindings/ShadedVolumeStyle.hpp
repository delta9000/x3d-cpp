#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ShadedVolumeStyle {
  static constexpr std::string_view x3d_name = "ShadedVolumeStyle";
  inline static constexpr field_key<ShadedVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, bool> lighting{
      "lighting", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      material{"material", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string>
      phaseFunction{"phaseFunction", access_type::initialize_only};
  inline static constexpr field_key<ShadedVolumeStyle, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      surfaceNormals{"surfaceNormals", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ShadedVolumeStyle, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
