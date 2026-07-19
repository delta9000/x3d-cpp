#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct EdgeEnhancementVolumeStyle {
  static constexpr std::string_view x3d_name = "EdgeEnhancementVolumeStyle";
  inline static constexpr field_key<EdgeEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::color4f>
      edgeColor{"edgeColor", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, float>
      gradientThreshold{"gradientThreshold", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      surfaceNormals{"surfaceNormals", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, std::string>
      DEF{"DEF", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, std::string>
      USE{"USE", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<EdgeEnhancementVolumeStyle, std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
