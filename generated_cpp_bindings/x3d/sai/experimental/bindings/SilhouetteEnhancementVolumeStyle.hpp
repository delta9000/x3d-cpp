#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SilhouetteEnhancementVolumeStyle {
  static constexpr std::string_view x3d_name =
      "SilhouetteEnhancementVolumeStyle";
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle, bool>
      enabled{"enabled", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle, float>
      silhouetteBoundaryOpacity{"silhouetteBoundaryOpacity",
                                access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle, float>
      silhouetteRetainedOpacity{"silhouetteRetainedOpacity",
                                access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle, float>
      silhouetteSharpness{"silhouetteSharpness", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      surfaceNormals{"surfaceNormals", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    std::string>
      DEF{"DEF", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    std::string>
      USE{"USE", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    std::string>
      id{"id", access_type::input_output};
  inline static constexpr field_key<SilhouetteEnhancementVolumeStyle,
                                    std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
