#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BoundaryEnhancementVolumeStyle {
  static constexpr std::string_view x3d_name = "BoundaryEnhancementVolumeStyle";
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      boundaryOpacity{"boundaryOpacity", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, bool>
      enabled{"enabled", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      opacityFactor{"opacityFactor", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      retainedOpacity{"retainedOpacity", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, std::string>
      DEF{"DEF", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, std::string>
      USE{"USE", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, std::string>
      id{"id", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
