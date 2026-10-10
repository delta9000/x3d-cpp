#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct BoundaryEnhancementVolumeStyle {
  static constexpr std::string_view x3d_name = "BoundaryEnhancementVolumeStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      boundaryOpacity{"boundaryOpacity", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, bool>
      enabled{"enabled", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      opacityFactor{"opacityFactor", access_type::input_output};
  inline static constexpr field_key<BoundaryEnhancementVolumeStyle, float>
      retainedOpacity{"retainedOpacity", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {boundaryOpacity.name(), boundaryOpacity.kind, boundaryOpacity.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {opacityFactor.name(), opacityFactor.kind, opacityFactor.access()},
      {retainedOpacity.name(), retainedOpacity.kind, retainedOpacity.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
